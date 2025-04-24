section .data
HexTable db "0123456789ABCDEF"

section .text
global _start

;;; ---------------------------------------------
;;; Descript:   print num
;;; Entry:      rax  = num
;;;             r11 = base
;;; Exit:       rax = exit code
;;; Destroy:    rcx, rdx, rsi, rdi, r11
;;; ---------------------------------------------
PrintNum:
    mov rdi, rax                            ; rdi - num

    xor rcx, rcx                            ; rcx - string size

;;; check to zero and negative
    test rax, rax
js .Negative
jne .Convertion
;;; push '0' in stack
    dec rsp
    mov byte [rsp], '0'
    inc rcx                                 ; ++size
jmp .Print

.Negative:
    neg rax                                 ; num = -num

.Convertion:
    xor rdx, rdx                            ; rdx = 0 (in particular edx)
    div r11                                 ; [rax, rdx] = rdx:rax / r11
    mov dl, byte [HexTable + rdx]           ; dl = HexTable[dl]
;;; push dl (digit) in stack
    dec rsp
    mov byte [rsp], dl

    inc rcx                                 ; ++size
    test rax, rax
jne .Convertion

;;; check to negative (add '-')
    test rdi, rdi
jns .Print
;;; push '-' in stack
    dec rsp
    mov byte [rsp], '-'
    inc rcx                                 ; ++size

.Print:

    mov rdx, rcx                            ; rdx - size string
    mov rsi, rsp                            ; rsi - addr string for print
    mov rdi, 1
    mov rax, 1
    syscall
    add rsp, rdx                            ; clean stack (rdx - size string)
    test rax, rax                           ; check error
je .Exit

.ExitSuccess:
    xor rax, rax                            ; NO ERROR
.Exit:
ret

_start:
call main
mov r11, 10
call PrintNum
mov rdi, rax
mov rax, 60
syscall

;;; COMMENT: main
main:
push rbp
mov rbp, rsp
;;; COMMENT: num
push qword 3
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: call func
call func_2_1
add rsp, 8
push rax
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: call func
call func_4_1
add rsp, 8
push rax
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: call func
call func_2_1
add rsp, 8
push rax
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret

func_2_1:
push rbp
mov rbp, rsp
push qword [rsp+16]
;;; COMMENT: if
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 1
;;; COMMENT: eq
pop rbx
pop rcx
cmp rcx, rbx
jne .NotEq0
push 1
jmp .EndEq0
.NotEq0:
push 0
.EndEq0:
pop rbx
test rbx, rbx
je .label1
;;; COMMENT: if->if
;;; COMMENT: num
push qword 1
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label2
.label1:
.label2:
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 1
;;; COMMENT: sub
pop rbx
pop rcx
sub rcx, rbx
push rcx
;;; COMMENT: call func
call func_2_1
add rsp, 8
push rax
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: mul
pop rbx
pop rcx
imul rcx, rbx
push rcx
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret

func_4_1:
push rbp
mov rbp, rsp
push qword [rsp+16]
;;; COMMENT: if
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 1
;;; COMMENT: eq
pop rbx
pop rcx
cmp rcx, rbx
jne .NotEq3
push 1
jmp .EndEq3
.NotEq3:
push 0
.EndEq3:
pop rbx
test rbx, rbx
je .label4
;;; COMMENT: if->if
;;; COMMENT: num
push qword 1
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label5
.label4:
.label5:
;;; COMMENT: if
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 0
;;; COMMENT: eq
pop rbx
pop rcx
cmp rcx, rbx
jne .NotEq6
push 1
jmp .EndEq6
.NotEq6:
push 0
.EndEq6:
pop rbx
test rbx, rbx
je .label7
;;; COMMENT: if->if
;;; COMMENT: num
push qword 0
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label8
.label7:
.label8:
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 1
;;; COMMENT: sub
pop rbx
pop rcx
sub rcx, rbx
push rcx
;;; COMMENT: call func
call func_4_1
add rsp, 8
push rax
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 2
;;; COMMENT: sub
pop rbx
pop rcx
sub rcx, rbx
push rcx
;;; COMMENT: call func
call func_4_1
add rsp, 8
push rax
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret
