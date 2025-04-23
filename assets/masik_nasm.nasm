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
push qword 25
;;; COMMENT: num
push qword 4
;;; COMMENT: num
push qword 3
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: pow
pop rcx
pop rdx
mov rbx, 1
test rcx, rcx
je .ZeroPow0
.HelpCycle0:
  imul rbx, rdx
loop .HelpCycle0
.ZeroPow0:
push rbx
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: pow
pop rcx
pop rdx
mov rbx, 1
test rcx, rcx
je .ZeroPow1
.HelpCycle1:
  imul rbx, rdx
loop .HelpCycle1
.ZeroPow1:
push rbx
;;; COMMENT: var
push qword [rbp-48]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: var
push qword [rbp-56]
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-8]
;;; COMMENT: var
push qword [rbp-16]
;;; COMMENT: div
xor rdx, rdx
pop rcx
pop rax
idiv rcx
push rax
;;; COMMENT: var
push qword [rbp-24]
;;; COMMENT: num
push qword 2
;;; COMMENT: pow
pop rcx
pop rdx
mov rbx, 1
test rcx, rcx
je .ZeroPow2
.HelpCycle2:
  imul rbx, rdx
loop .HelpCycle2
.ZeroPow2:
push rbx
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-32]
;;; COMMENT: var
push qword [rbp-40]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-48]
;;; COMMENT: sum
pop rbx
pop rcx
add rcx, rbx
push rcx
;;; COMMENT: var
push qword [rbp-56]
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
;;; COMMENT: var
push qword [rbp-72]
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
