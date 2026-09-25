--  engine/integrity.adb
--
--  HyperTension Exhaustive Integrity Authority.
--
--  PROCESS COMMUNICATION PROTOCOL
--  ================================
--  Standard input (one line):
--    {target} {dataset_size} {present} {index} {membership} {agreement}
--    {conf_bp} {admissible} {seal_hex} {test_mode}
--
--    present, membership, agreement, admissible: 0 (false) or 1 (true)
--    index: -1 if the target is absent, otherwise the 0-based result index
--    seal_hex: exactly 8 hexadecimal characters (the Forth verification seal)
--    test_mode: 1 to use reduced dimensions for unit tests, 0 for production
--
--  Standard output (one line):
--    {total_states} {valid_states} {corrupted_detected} PASS
--    or
--    {total_states} {valid_states} {corrupted_detected} FAIL
--
--  Standard error (zero or more lines):
--    PROGRESS {n}   emitted periodically during production runs

with Ada.Text_IO;
with Ada.Characters.Handling;
with Interfaces;                     use Interfaces;

procedure Integrity is

   --  32-bit unsigned modular arithmetic.  Overflow wraps modulo 2^32.
   subtype U32 is Unsigned_32;

   --  Confidence in basis points.  Range constraint enforces [0, 10000].
   subtype Confidence_BP_Range is Integer range 0 .. 10_000;

   --  -1 is the canonical sentinel for absence; all other values are 0-based.
   subtype Result_Index_Type is Integer range -1 .. Integer'Last;

   type Evidence_Record is record
      Target        : Integer;
      Dataset_Size  : Positive;
      Present       : Boolean;
      Result_Index  : Result_Index_Type;
      Membership    : Boolean;
      Agreement     : Boolean;
      Conf_BP       : Confidence_BP_Range;
      Admissible    : Boolean;
      Seal          : U32;
   end record;

   --  Production: 8192 × 2048 = 16,777,216 outer states × 128 inner steps.
   --  Estimated runtime on a 3 GHz machine: approximately 4–8 seconds.
   SEAL_STEPS_PROD    : constant := 8_192;
   EVID_VARIANTS_PROD : constant := 2_048;
   INNER_STEPS_PROD   : constant := 128;

   --  Reduced dimensions for unit tests.
   SEAL_STEPS_TEST    : constant := 16;
   EVID_VARIANTS_TEST : constant := 8;
   INNER_STEPS_TEST   : constant := 4;

   Progress_Interval : constant := 1_048_576;

   --  Shared with the Forth finalization stage for cross-stage consistency.
   LCG_Prime : constant U32 := 2_103_515_245;

   Knuth_Factor : constant U32 := 2_654_435_761;

   function Mix (Acc, Val : U32) return U32 is
   begin
      return Acc xor (Val * LCG_Prime);
   end Mix;

   --  LCG_Prime is odd, therefore invertible modulo 2^32.  Two chains
   --  started from distinct seeds will diverge on the first step and
   --  remain distinct on every subsequent step.  The invariant
   --  (Running_Ev /= Running_Canon) therefore holds for all V > 0.
   function Chain_Step (R, DS, Sub : U32) return U32 is
   begin
      return (R * LCG_Prime) xor (DS + Sub);
   end Chain_Step;

   --  Ada has no rotate primitive.  N mod 32 prevents a shift-by-32
   --  on targets where that amount is undefined.
   function Rotate_Left_32 (X : U32; N : Natural) return U32 is
      N_Mod : constant Natural := N mod 32;
   begin
      if N_Mod = 0 then
         return X;
      end if;
      return Shift_Left (X, N_Mod) or Shift_Right (X, 32 - N_Mod);
   end Rotate_Left_32;

   function Derived_Seal (Base : U32; Step : Natural) return U32 is
   begin
      return Base xor (U32 (Step) * Knuth_Factor);
   end Derived_Seal;

   --  Every field participates.  Booleans are encoded as distinct odd
   --  integers so neither truth value reduces to a zero multiplicand.
   function Evidence_Hash (Ev : Evidence_Record) return U32 is
      H : U32 := Ev.Seal;
   begin
      --  Offset target to avoid degenerate zero interaction.
      H := Mix (H, U32 (Ev.Target + 1_000_000));

      --  Confidence.
      H := Mix (H, U32 (Ev.Conf_BP));

      H := Mix (H, U32 (Ev.Dataset_Size));

      --  Sentinel -1 maps to 0; 0-based indices map to 1+.  All distinct.
      H := Mix (H, U32 (Ev.Result_Index + 1));

      H := Mix (H, (if Ev.Present    then 3  else 7));
      H := Mix (H, (if Ev.Membership then 11 else 13));
      H := Mix (H, (if Ev.Agreement  then 17 else 19));
      H := Mix (H, (if Ev.Admissible then 23 else 29));

      H := Rotate_Left_32 (H, 7);
      return H;
   end Evidence_Hash;

   --  Apply perturbation V to produce a virtual evidence record.
   --  V = 0 returns the canonical record unchanged.
   --
   --  Encoding of V:
   --    bits 0..3   boolean field flips (membership, agreement, admissible, present)
   --    bits 4..7   target offset 0..15
   --    bits 8..10  confidence offset: ((V / 256) mod 8) × 100 basis points
   --
   --  For V ∈ [1, 2047] at least one field differs from the canonical record.
   --  (The group where bits 0..7 are zero begins at V = 256, which still
   --   carries a non-zero confidence offset.)
   function Perturb (Canon : Evidence_Record; V : Natural) return Evidence_Record is
      Result          : Evidence_Record := Canon;
      Target_Offset   : constant Integer := Integer ((V / 16) mod 16);
      Conf_Offset     : constant Integer := Integer ((V / 256) mod 8) * 100;
      New_Conf        : Integer;
   begin
      if (V mod 2) /= 0 then
         Result.Membership := not Result.Membership;
      end if;
      if (V / 2 mod 2) /= 0 then
         Result.Agreement := not Result.Agreement;
      end if;
      if (V / 4 mod 2) /= 0 then
         Result.Admissible := not Result.Admissible;
      end if;
      if (V / 8 mod 2) /= 0 then
         Result.Present := not Result.Present;
      end if;

      Result.Target := Canon.Target + Target_Offset;

      New_Conf := Integer (Canon.Conf_BP) + Conf_Offset;
      if New_Conf > 10_000 then
         Result.Conf_BP := 10_000;
      else
         Result.Conf_BP := Confidence_BP_Range (New_Conf);
      end if;

      return Result;
   end Perturb;

   function Parse_Hex_U32 (S : String) return U32 is
      Result : U32  := 0;
      C      : Character;
      Digit  : U32;
   begin
      for I in S'Range loop
         Result := Shift_Left (Result, 4);
         C := Ada.Characters.Handling.To_Upper (S (I));
         case C is
            when '0' .. '9' =>
               Digit := U32 (Character'Pos (C) - Character'Pos ('0'));
            when 'A' .. 'F' =>
               Digit := U32 (Character'Pos (C) - Character'Pos ('A') + 10);
            when others =>
               raise Constraint_Error
                  with "invalid hexadecimal digit in seal";
         end case;
         Result := Result or Digit;
      end loop;
      return Result;
   end Parse_Hex_U32;

   --  Long_Long_Integer'Image always prepends a space; strip it.
   procedure Put_LLI (N : Long_Long_Integer) is
      S : constant String := Long_Long_Integer'Image (N);
   begin
      Ada.Text_IO.Put (S (S'First + 1 .. S'Last));
   end Put_LLI;

   procedure Parse_Input (
      Input     :     String;
      Canon     : out Evidence_Record;
      Test_Mode : out Boolean)
   is
      Pos : Integer := Input'First;

      procedure Skip_WS is
      begin
         while Pos <= Input'Last and then
               (Input (Pos) = ' ' or else Input (Pos) = ASCII.HT) loop
            Pos := Pos + 1;
         end loop;
      end Skip_WS;

      --  Read a signed decimal integer.
      function Read_Int return Integer is
         Neg   : Boolean := False;
         Start : Integer;
         Value : Integer := 0;
      begin
         Skip_WS;
         if Pos <= Input'Last and then Input (Pos) = '-' then
            Neg := True;
            Pos := Pos + 1;
         end if;
         Start := Pos;
         while Pos <= Input'Last and then Input (Pos) in '0' .. '9' loop
            Value := Value * 10 +
               (Character'Pos (Input (Pos)) - Character'Pos ('0'));
            Pos := Pos + 1;
         end loop;
         if Pos = Start then
            raise Constraint_Error with "expected integer in input";
         end if;
         return (if Neg then -Value else Value);
      end Read_Int;

      --  Read exactly 8 hexadecimal characters.
      function Read_Hex_Seal return U32 is
         Seal_Str : String (1 .. 8);
      begin
         Skip_WS;
         if Pos + 7 > Input'Last then
            raise Constraint_Error with "seal field too short in input";
         end if;
         Seal_Str := Input (Pos .. Pos + 7);
         Pos := Pos + 8;
         return Parse_Hex_U32 (Seal_Str);
      end Read_Hex_Seal;

      Target_V, Ds_V, Present_V, Index_V,
      Membership_V, Agreement_V, Conf_V,
      Admissible_V, Test_V : Integer;

   begin
      Target_V     := Read_Int;
      Ds_V         := Read_Int;
      Present_V    := Read_Int;
      Index_V      := Read_Int;
      Membership_V := Read_Int;
      Agreement_V  := Read_Int;
      Conf_V       := Read_Int;
      Admissible_V := Read_Int;

      declare
         Seal_Val : constant U32 := Read_Hex_Seal;
      begin
         Test_V := Read_Int;

         Canon.Target       := Target_V;
         Canon.Dataset_Size := Positive (Ds_V);
         Canon.Present      := Present_V /= 0;
         Canon.Result_Index := Result_Index_Type (Index_V);
         Canon.Membership   := Membership_V /= 0;
         Canon.Agreement    := Agreement_V /= 0;
         Canon.Conf_BP      := Confidence_BP_Range (Conf_V);
         Canon.Admissible   := Admissible_V /= 0;
         Canon.Seal         := Seal_Val;
         Test_Mode          := Test_V /= 0;
      end;
   end Parse_Input;

   Canon            : Evidence_Record;
   Test_Mode        : Boolean := False;
   Canon_Hash       : U32;

   Seal_Steps    : Natural;
   Evid_Variants : Natural;
   Inner_Steps   : Natural;

   --  Total_Outer:      (seal_step, evidence_variant) pairs examined.
   --  Valid_Outer:      canonical states (V = 0) where all inner checks passed.
   --  Corrupt_Detected: non-canonical states where at least one check failed.
   --  Failure_Count:    states that violated the expected classification.
   Total_Outer      : Long_Long_Integer := 0;
   Valid_Outer      : Long_Long_Integer := 0;
   Corrupt_Detected : Long_Long_Integer := 0;
   Failure_Count    : Long_Long_Integer := 0;

begin
   --  Parse it.
   declare
      Line : String (1 .. 512);
      Last : Natural;
   begin
      Ada.Text_IO.Get_Line (Line, Last);
      Parse_Input (Line (1 .. Last), Canon, Test_Mode);
   end;

   if Test_Mode then
      Seal_Steps    := SEAL_STEPS_TEST;
      Evid_Variants := EVID_VARIANTS_TEST;
      Inner_Steps   := INNER_STEPS_TEST;
   else
      Seal_Steps    := SEAL_STEPS_PROD;
      Evid_Variants := EVID_VARIANTS_PROD;
      Inner_Steps   := INNER_STEPS_PROD;
   end if;

   --  Anchors the invariant: V = 0 chains are identical; V > 0 chains diverge.
   Canon_Hash := Evidence_Hash (Canon);

   for S in 0 .. Seal_Steps - 1 loop
      declare
         DS : constant U32 := Derived_Seal (Canon.Seal, S);
      begin
         for V in 0 .. Evid_Variants - 1 loop
            declare
               Ev            : constant Evidence_Record := Perturb (Canon, V);
               Ev_Hash       : constant U32 := Evidence_Hash (Ev);
               Running_Ev    : U32     := Ev_Hash;
               Running_Canon : U32     := Canon_Hash;
               Any_Fail      : Boolean := False;
            begin
               --  Increment the state counter.
               Total_Outer := Total_Outer + 1;

               --  The loop does not exit early.  All Inner_Steps iterations
               --  execute regardless of Any_Fail; this is load-bearing for
               --  computational cost on non-canonical states.
               for Sub in 0 .. Inner_Steps - 1 loop
                  declare
                     Sub_U : constant U32 := U32 (Sub + 1);
                  begin
                     Running_Ev    := Chain_Step (Running_Ev,    DS, Sub_U);
                     Running_Canon := Chain_Step (Running_Canon, DS, Sub_U);

                     --  V = 0: chains are identical; this is never true.
                     --  V > 0: chains diverge on step 1 (LCG_Prime invertible
                     --  mod 2^32) and remain distinct thereafter.
                     if Running_Ev /= Running_Canon then
                        Any_Fail := True;
                     end if;
                  end;
               end loop;

               if V = 0 then
                  if Any_Fail then
                     Failure_Count := Failure_Count + 1;
                  else
                     Valid_Outer := Valid_Outer + 1;
                  end if;
               else
                  if Any_Fail then
                     Corrupt_Detected := Corrupt_Detected + 1;
                  else
                     --  Corrupted state escaped detection.
                     Failure_Count := Failure_Count + 1;
                  end if;
               end if;
            end;
         end loop;

         --  Progress.
         if (not Test_Mode)
            and then (Total_Outer mod Long_Long_Integer (Progress_Interval) = 0)
         then
            declare
               S_Img : constant String :=
                  Long_Long_Integer'Image (Total_Outer);
            begin
               Ada.Text_IO.Put_Line
                  (Ada.Text_IO.Standard_Error,
                   "PROGRESS " & S_Img (S_Img'First + 1 .. S_Img'Last));
            end;
         end if;
      end;
   end loop;

   --  Pass requires zero failures and exactly Seal_Steps valid canonical states.
   --  The second condition is implied by the first when the loop completes, but
   --  is checked independently as a guard.
   declare
      Pass : constant Boolean :=
         (Failure_Count = 0)
         and then (Valid_Outer = Long_Long_Integer (Seal_Steps));
   begin
      Put_LLI (Total_Outer);
      Ada.Text_IO.Put (" ");
      Put_LLI (Valid_Outer);
      Ada.Text_IO.Put (" ");
      Put_LLI (Corrupt_Detected);
      Ada.Text_IO.Put (" ");
      if Pass then
         Ada.Text_IO.Put_Line ("PASS");
      else
         Ada.Text_IO.Put_Line ("FAIL");
      end if;
   end;
end Integrity;
