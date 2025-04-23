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
push qword 1500
;;; COMMENT: num
push qword 3
;;; COMMENT: num
push qword 22
;;; COMMENT: num
push qword 1
;;; COMMENT: num
push qword 0
;;; COMMENT: num
push qword 0
;;; COMMENT: if;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: num
push qword 18
;;; COMMENT: great
pop rbx
pop rcx
cmp rcx, rbx
jle .NotGreat0
 push 1
jmp .EndGreat0
.NotGreat0:
push 0
.EndGreat0:
pop rbx
test rbx, rbx
je .label1
;;; COMMENT: if->if;;; COMMENT: num
push qword 1
;;; COMMENT: assign
pop qword [rbp-40]
;;; COMMENT: if;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 1000
;;; COMMENT: great
pop rbx
pop rcx
cmp rcx, rbx
jle .NotGreat2
 push 1
jmp .EndGreat2
.NotGreat2:
push 0
.EndGreat2:
pop rbx
test rbx, rbx
je .label3
;;; COMMENT: if->if;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: num
push qword 5
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: if;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: num
push qword 3
;;; COMMENT: eq
pop rbx
pop rcx
cmp rcx, rbx
jne .NotEq4
 push 1
jmp .EndEq4
.NotEq4:
push 0
.EndEq4:
pop rbx
test rbx, rbx
je .label5
;;; COMMENT: if->if;;; COMMENT: var
push qword [rbp-56]
;;; COMMENT: num
push qword 2
;;; COMMENT: mul
pop rbx
pop rcx
imul rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-48]
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: var
push qword [rbp-48]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-8]
jmp .label6
.label5:
;;; COMMENT: var
push qword [rbp-56]
;;; COMMENT: num
push qword 50
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: assign
pop qword [rbp-48]
.label6:
jmp .label7
.label3:
;;; COMMENT: num
push qword 0
;;; COMMENT: assign
pop qword [rbp-48]
.label7:
jmp .label8
.label1:
;;; COMMENT: num
push qword 0
;;; COMMENT: assign
pop qword [rbp-40]
;;; COMMENT: num
push qword 0
;;; COMMENT: assign
pop qword [rbp-48]
.label8:
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: var
push qword [rbp-48]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: mul
pop rbx
pop rcx
imul rcx, rbx
push rcx
;;; COMMENT: if;;; COMMENT: var
push qword [rbp-64]
;;; COMMENT: num
push qword 2000
;;; COMMENT: great
pop rbx
pop rcx
cmp rcx, rbx
jle .NotGreat9
 push 1
jmp .EndGreat9
.NotGreat9:
push 0
.EndGreat9:
pop rbx
test rbx, rbx
je .label10
;;; COMMENT: if->if;;; COMMENT: num
push qword 1
;;; COMMENT: assign
pop qword [rbp-32]
jmp .label11
.label10:
.label11:
;;; COMMENT: var
push qword [rbp-72]
;;; COMMENT: var
push qword [rbp-32]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-64]
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
