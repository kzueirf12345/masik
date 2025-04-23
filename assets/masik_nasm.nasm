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
push qword 30
push qword 50
push qword 70
push qword 2
push qword 3
push qword 10
push qword [rbp-32]
push qword [rbp-8]
pop rbx
pop rcx
imul rcx, rbx
push rcx
push qword [rbp-40]
push qword [rbp-16]
pop rbx
pop rcx
imul rcx, rbx
push rcx
push qword [rbp-48]
push qword [rbp-24]
pop rbx
pop rcx
imul rcx, rbx
push rcx
push qword [rbp-56]
push qword [rbp-64]
pop rbx
pop rcx
add rcx, rbx
push rcx
push qword [rbp-72]
pop rbx
pop rcx
add rcx, rbx
push rcx
push qword [rbp-80]
push qword 10
pop rbx
pop rcx
imul rcx, rbx
push rcx
push qword [rbp-80]
push qword [rbp-88]
push qword 20
pop rbx
pop rcx
imul rcx, rbx
push rcx
pop rbx
pop rcx
sub rcx, rbx
push rcx
push qword [rbp-80]
push qword [rbp-88]
pop rbx
pop rcx
sub rcx, rbx
push rcx
push qword [rbp-96]
pop rbx
pop rcx
add rcx, rbx
push rcx
push qword [rbp-80]
push qword 10
pop rbx
pop rcx
imul rcx, rbx
push rcx
push qword [rbp-112]
push qword [rbp-104]
pop rbx
pop rcx
sub rcx, rbx
push rcx
push qword 100
pop rbx
pop rcx
add rcx, rbx
push rcx
push qword [rbp-104]
push qword [rbp-120]
push qword [rbp-128]
push qword [rbp-136]
pop rbx
pop rcx
add rcx, rbx
push rcx
push qword 50
pop rbx
pop rcx
sub rcx, rbx
push rcx
pop rax
mov rsp, rbp
pop rbp
ret
