section .data
HexTable db "0123456789ABCDEF"

section .text
global _start

_start:
call main
push rax
call hlt
add rsp, 8

main:
pop rax ; save ret val
mov rbx, rbp ; save old rbp
mov rbp, rsp
add rbp, 0
 ; rbp = rsp + arg_cnt
mov rsp, rbp
sub rsp, 24 ; rsp = rbp - local_vars_cnt
push rax ; ret val
push rbx ; old rbp
push 10
pop qword [rbp-8]
push qword [rbp-8]
call out
add rsp, 8
push qword [rbp-8]
call func_2_1
push rax
pop qword [rbp-16]
push qword [rbp-16]
call out
add rsp, 8
push qword [rbp-8]
call func_4_1
push rax
pop qword [rbp-24]
push qword [rbp-24]
call out
add rsp, 8
push 0
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret


func_2_1:
pop rax ; save ret val
mov rbx, rbp ; save old rbp
mov rbp, rsp
add rbp, 8
 ; rbp = rsp + arg_cnt
mov rsp, rbp
sub rsp, 8 ; rsp = rbp - local_vars_cnt
push rax ; ret val
push rbx ; old rbp
push qword [rbp-8]
push 1
pop rcx
pop rbx
cmp rbx, rcx
sete bl
movzx rbx, bl
push rbx
pop rbx
test rbx, rbx
jne label1

mov rbx, 1
test rbx, rbx
jne label0

label1:
push 1
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label0:
push qword [rbp-8]
push 1
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_2_1
push rax
push qword [rbp-8]
pop rcx
pop rbx
imul rbx, rcx
push rbx
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret


func_4_1:
pop rax ; save ret val
mov rbx, rbp ; save old rbp
mov rbp, rsp
add rbp, 8
 ; rbp = rsp + arg_cnt
mov rsp, rbp
sub rsp, 8 ; rsp = rbp - local_vars_cnt
push rax ; ret val
push rbx ; old rbp
push qword [rbp-8]
push 1
pop rcx
pop rbx
cmp rbx, rcx
sete bl
movzx rbx, bl
push rbx
pop rbx
test rbx, rbx
jne label3

mov rbx, 1
test rbx, rbx
jne label2

label3:
push 1
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label2:
push qword [rbp-8]
push 0
pop rcx
pop rbx
cmp rbx, rcx
sete bl
movzx rbx, bl
push rbx
pop rbx
test rbx, rbx
jne label5

mov rbx, 1
test rbx, rbx
jne label4

label5:
push 0
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label4:
push qword [rbp-8]
push 1
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_4_1
push rax
push qword [rbp-8]
push 2
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_4_1
push rax
pop rcx
pop rbx
add rbx, rcx
push rbx
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

hlt:
mov rdi, [rsp + 8]
mov rax, 60
syscall

in:
push 22866613
call hlt
add rsp, 8
ret

;;; ---------------------------------------------
;;; Descript:   print num
;;; Entry:      first pushed arg  = num
;;; Exit:       rax = exit code
;;; Destroy:    rcx, rdx, rsi, rdi, r11
;;; ---------------------------------------------
out:
    mov rax, [rsp + 8]                      ; rax - num
    mov r11, 10                             ; r11 - base

    mov rdi, rax                            ; rdi - num

    xor rcx, rcx                            ; rcx - string size
;;; add \n 
    dec rsp
    mov byte [rsp], `\n`
    inc rcx                                 ; ++size

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

;;; ---------------------------------------------
;;; Descript:   fast power calculation (x^n)
;;; Entry:      first pushed arg  = x (base)
;;;             second pushed arg = n (exponent)
;;; Exit:       rax = res 
;;; Destroy:    rcx, rdx
;;; ---------------------------------------------
pow:
    mov rcx, [rsp + 8]                      ; rcx - n
    mov rax, [rsp + 16]                     ; rax - x
    mov rdx, 1                              ; rdx = 1 - result

;;; Проверка особых случаев
    test rcx, rcx                           ; n == 0
    je .done

    cmp rax, 1                              ; x == 1
    je .done

    test rax, rax                           ; x == 0
    je .zero_case

.pow_loop:
    test rcx, 1                             ; check even
    jz .even_power
    imul rdx, rax                           ; res *= x 
    dec rcx                                 ; --n
    jz .done                                ; n == 0
.even_power:
    imul rax, rax                           ; x *= x
    shr rcx, 1                              ; n /= 2
    jnz .pow_loop                           ; n != 0

.done:
    mov rax, rdx                            ; rax - ret val
    ret

.zero_case:
    xor rax, rax                            ; rax = 0 - ret val
    ret

