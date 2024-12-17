PUSH 0
POP [0]
PUSH 1
POP [1]
PUSH 1
POP [2]
PUSH [2]
PUSH [0]
LEQ
PUSH 0
JE :label0
JMP :label1
:label2
PUSH [2]
PUSH [0]
LEQ
PUSH 0
JE :label3
:label1
PUSH [1]
PUSH [2]
MUL
POP [1]
PUSH [2]
PUSH 1
ADD
POP [2]
JMP :label2
:label0
:label3
PUSH [1]
OUT
HLT
