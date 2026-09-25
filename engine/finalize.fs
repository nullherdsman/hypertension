\ HyperTension Record Finalization Authority
\ yeah good luck on this one, no comments at all

VARIABLE SEAL-BASE

: MIX  ( acc val -- acc' )
    2103515245 * $FFFFFFFF AND
    XOR ;

: ROT13  ( n -- n' )
    $FFFFFFFF AND
    DUP 13 LSHIFT $FFFFFFFF AND
    SWAP 19 RSHIFT
    OR ;

: FOLD  ( acc val -- acc' )
    MIX ROT13 ;

: SEAL  ( t d p i m a c x -- seal )
    0
    8 ROLL FOLD
    7 ROLL FOLD
    6 ROLL FOLD
    5 ROLL FOLD
    4 ROLL FOLD
    3 ROLL FOLD
    2 ROLL FOLD
    1 ROLL FOLD
    $FFFFFFFF AND ;

: .SEAL  ( seal -- )
    $FFFFFFFF AND S>D
    BASE @ SEAL-BASE !
    HEX
    <# # # # # # # # # #>
    SEAL-BASE @ BASE !
    TYPE CR ;

: FINALIZE  ( t d p i m a c x -- )
    SEAL .SEAL BYE ;
