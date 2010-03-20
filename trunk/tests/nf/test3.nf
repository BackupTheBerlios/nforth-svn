( Test various kinds of loops )
: X 10 1 ?DO DUP . LOOP 'X' EMIT ;
: Y 1 BEGIN DUP . 1+ 10 CHEQ UNTIL DROP 'Y' EMIT ;
: Z 1 BEGIN DUP 10 < WHILE DUP . 1+ REPEAT DROP 'Z' EMIT ;
X Y Z .S
