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

main:
push rbp
mov rbp, rsp
push qword 3
push qword [rbp-8]
call func_2_1
add rsp, 8
push rax
push qword [rbp-16]
call func_4_1
add rsp, 8
push rax
push qword [rbp-24]
call func_2_1
add rsp, 8
push rax
pop rax
mov rsp, rbp
pop rbp
ret

func_2_1:
push rbp
mov rbp, rsp
push qword [rsp+16]
push qword [rbp-8]
push qword 1
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
push qword 1
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label2
.label1:
.label2:
push qword [rbp-8]
push qword 1
pop rbx
pop rcx
sub rcx, rbx
push rcx
call func_2_1
add rsp, 8
push rax
push qword [rbp-8]
pop rbx
pop rcx
imul rcx, rbx
push rcx
pop rax
mov rsp, rbp
pop rbp
ret

func_4_1:
push rbp
mov rbp, rsp
push qword [rsp+16]
push qword [rbp-8]
push qword 1
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
push qword 1
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label5
.label4:
.label5:
push qword [rbp-8]
push qword 0
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
push qword 0
pop rax
mov rsp, rbp
pop rbp
ret
jmp .label8
.label7:
.label8:
push qword [rbp-8]
push qword 1
pop rbx
pop rcx
sub rcx, rbx
push rcx
call func_4_1
add rsp, 8
push rax
push qword [rbp-8]
push qword 2
pop rbx
pop rcx
sub rcx, rbx
push rcx
call func_4_1
add rsp, 8
push rax
pop rbx
pop rcx
add rcx, rbx
push rcx
pop rax
mov rsp, rbp
pop rbp
ret
