PUSH 0
POP R1
PUSH 0
POP R2
PUSH 0
CALL :main
POP R1
OUT
HLT
:main
PUSH 5
POP [0+R1]
PUSH R1
PUSH [0+R1]
PUSH R1
PUSH 1
ADD
POP R1
CALL :func_0_1
POP R1
POP R2
POP R1
PUSH R2
PUSH R1
RET
RET

:func_0_1
POP [0+R1]
PUSH [0+R1]
PUSH 1
EQ
PUSH 0
JE :label0
PUSH 1
POP R2
POP R1
PUSH R2
PUSH R1
RET
JMP :label1
:label0
:label1
PUSH R1
PUSH [0+R1]
PUSH 1
SUB
PUSH R1
PUSH 1
ADD
POP R1
CALL :func_0_1
POP R1
PUSH [0+R1]
MUL
POP R2
POP R1
PUSH R2
PUSH R1
RET
RET

:func_2_1
POP [0+R1]
PUSH [0+R1]
PUSH 1
EQ
PUSH 0
JE :label2
PUSH 1
POP R2
POP R1
PUSH R2
PUSH R1
RET
JMP :label3
:label2
:label3
PUSH [0+R1]
PUSH 0
EQ
PUSH 0
JE :label4
PUSH 0
POP R2
POP R1
PUSH R2
PUSH R1
RET
JMP :label5
:label4
:label5
PUSH R1
PUSH [0+R1]
PUSH 1
SUB
PUSH R1
PUSH 1
ADD
POP R1
CALL :func_2_1
POP R1
PUSH R1
PUSH [0+R1]
PUSH 2
SUB
PUSH R1
PUSH 1
ADD
POP R1
CALL :func_2_1
POP R1
ADD
POP R2
POP R1
PUSH R2
PUSH R1
RET
RET

