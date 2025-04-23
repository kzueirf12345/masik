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
push qword 0
;;; COMMENT: num
push qword 5
;;; COMMENT: num
push qword 0
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: less
pop rbx
pop rcx
cmp rcx, rbx
jge .NotLess0
 push 1
jmp .EndLess0
.NotLess0:
push 0
.EndLess0:
pop rbx
test rbx, rbx
je .label1
jmp .label2
.label3:
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: less
pop rbx
pop rcx
cmp rcx, rbx
jge .NotLess4
 push 1
jmp .EndLess4
.NotLess4:
push 0
.EndLess4:
pop rbx
test rbx, rbx
je .label5
.label2:
;;; COMMENT: num
push qword 1
;;; COMMENT: num
push qword 5
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: num
push qword 1
;;; COMMENT: great
pop rbx
pop rcx
cmp rcx, rbx
jle .NotGreat6
 push 1
jmp .EndGreat6
.NotGreat6:
push 0
.EndGreat6:
pop rbx
test rbx, rbx
je .label7
jmp .label8
.label9:
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: num
push qword 1
;;; COMMENT: great
pop rbx
pop rcx
cmp rcx, rbx
jle .NotGreat10
 push 1
jmp .EndGreat10
.NotGreat10:
push 0
.EndGreat10:
pop rbx
test rbx, rbx
je .label11
.label8:
;;; COMMENT: var
push qword [rbp-32]
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: mul
pop rbx
pop rcx
imul rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-32]
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: num
push qword 1
;;; COMMENT: sub
pop rbx
pop rcx
sub rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-40]
jmp .label9
.label7:
.label11:
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: var
push qword [rbp-32]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-8]
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: num
push qword 1
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-24]
jmp .label3
.label1:
.label5:
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: ret
pop rax
mov rsp, rbp
pop rbp
ret
