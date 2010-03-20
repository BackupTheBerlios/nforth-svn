VARIABLE ledn
VARIABLE dir

( Light the next LED )
: S-LED
    ledn @ 0 LED
    ledn @ dir @ +
    9 CHEQ IF -1 dir ! THEN
    0 CHEQ IF +1 dir ! THEN
    DUP ledn ! 1 LED
;

( Display ADC value and show a bar )
: S-ADC
    ADC DUP
    ." \r[ "
    .
    ." ]"
    16 /
    64 0 ?DO
        '-' OVER 4 PICK =
        IF DROP '|' THEN
        EMIT
    LOOP
    DROP
;

( Now both together )
: SHOW
    0 ledn !
    1 dir !
    BEGIN
        S-LED
        50 DELAY
        S-ADC
    REPEAT
;
