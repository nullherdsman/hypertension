C     HyperTension Statistical Confidence Authority
C     FORTRAN 77 fixed-form source
C     --------------------------------------------------------
C     Input (stdin):  N CFND BFPOS AGREE CIDX AIDX
C       N     - dataset size (INTEGER)
C       CFND  - C++ found flag: 1=found, 0=absent (INTEGER)
C       BFPOS - Brainfuck positive: 1=yes, 0=no (INTEGER)
C       AGREE - C++/ALGOL agreement: 1=yes, 0=no (INTEGER)
C       CIDX  - C++ candidate index, -1 if absent (INTEGER)
C       AIDX  - ALGOL candidate index, -1 if absent (INTEGER)
C     Output (stdout): confidence percentage, e.g. 99.7426
C     --------------------------------------------------------
C     Confidence model:
C       U  = BASE                           (initial uncertainty)
C          * RAGREE  (when AGREE=1)         (structural agreement)
C          * RBFYES  (when BF matches)      (clean evidence)
C          * RBFNO   (when BF mismatches)   (inconclusive evidence)
C          / (1.0 + RNDIV * N)              (dataset size factor)
C       confidence = 100.0 * (1.0 - U), clamped to [0, 100]
C
C     Constants: BASE=0.25  RAGREE=0.05  RBFYES=0.35
C                RBFNO=2.00  RNDIV=0.10
C
C     BF "matches" when BF evidence aligns with search result:
C       found  + BF positive  -> matches  (member confirmed)
C       absent + BF negative  -> matches  (absence confirmed)
C       absent + BF positive  -> mismatch (Bloom false positive)
C     --------------------------------------------------------
      PROGRAM CONFID
      INTEGER N, CFND, BFPOS, AGREE, CIDX, AIDX
      DOUBLE PRECISION U, CONFD
      DOUBLE PRECISION BASE, RAGREE, RBFYES, RBFNO, RNDIV
      PARAMETER (BASE   = 0.25D0)
      PARAMETER (RAGREE = 0.05D0)
      PARAMETER (RBFYES = 0.35D0)
      PARAMETER (RBFNO  = 2.00D0)
      PARAMETER (RNDIV  = 0.10D0)

      READ(*,*) N, CFND, BFPOS, AGREE, CIDX, AIDX

      U = BASE

      IF (AGREE .EQ. 1) THEN
          U = U * RAGREE
      END IF

      IF (CFND .EQ. BFPOS) THEN
          U = U * RBFYES
      ELSE
          U = U * RBFNO
      END IF

      U = U / (1.0D0 + RNDIV * DBLE(N))

      CONFD = 100.0D0 * (1.0D0 - U)

      IF (CONFD .GT. 100.0D0) CONFD = 100.0D0
      IF (CONFD .LT.   0.0D0) CONFD = 0.0D0

      WRITE(*,'(F8.4)') CONFD

      STOP
      END
