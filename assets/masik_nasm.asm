section .data
HexTable db "0123456789ABCDEF"
InputBufferSize equ 32
InputBuffer: times InputBufferSize db 0
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
push 0
pop qword [rbp-16]
label0:
push qword [rbp-16]
push qword [rbp-8]
pop rcx
pop rbx
cmp rbx, rcx
setl bl
movzx rbx, bl
push rbx
pop rbx
test rbx, rbx
jne label1

mov rbx, 1
test rbx, rbx
jne label2

label1:
push qword [rbp-16]
call func_3_1
push rax
pop qword [rbp-24]
push qword [rbp-24]
call out
add rsp, 8
push 1
push qword [rbp-16]
pop rcx
pop rbx
add rbx, rcx
push rbx
pop qword [rbp-16]
mov rbx, 1
test rbx, rbx
jne label0

label2:
push 0
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret


func_3_1:
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
jne label4

mov rbx, 1
test rbx, rbx
jne label3

label4:
push 1
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label3:
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
jne label6

mov rbx, 1
test rbx, rbx
jne label5

label6:
push 1
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label5:
push qword [rbp-8]
push 1
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_3_1
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


func_5_1:
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
jne label8

mov rbx, 1
test rbx, rbx
jne label7

label8:
push 1
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label7:
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
jne label10

mov rbx, 1
test rbx, rbx
jne label9

label10:
push 0
pop rax ; save ret val
pop rbx ; rbp val
pop rcx ; ret addr
mov rsp, rbp
mov rbp, rbx
push rcx
ret

label9:
push qword [rbp-8]
push 1
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_5_1
push rax
push qword [rbp-8]
push 2
pop rcx
pop rbx
sub rbx, rcx
push rbx
call func_5_1
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

;;; ---------------------------------------------
;;; Descript:   read num
;;; Entry:      NONE
;;; Exit:       rax = read num (0 if error)
;;; Destroy:    rcx, rdx, rsi, rdi, r11
;;; ---------------------------------------------
in:
    mov rsi, InputBuffer                       ; rsi - buffer addr
    mov rdx, InputBufferSize                   ; rdx - buffer size
    mov r10, 10                                ; r10 - base

    xor rax, rax                               ; sys_read
    xor rdi, rdi                               ; stdin
    syscall

    test rax, rax                              ; check read result
    jle .ExitError

    mov rcx, rax                               ; rcx - cnt bytes read
    dec rcx                                    ; not handle \n
    xor rax, rax                               ; rax - result num
    xor rdi, rdi                               ; rdi - sign flag (0 = positive)

;;; check first symbol
    mov bl, byte [rsi]
    cmp bl, '-'
    jne .CheckDigit
    inc rdi                                    ; set negative flag
    inc rsi                                    ; skip '-'
    dec rcx
    jz .ExitError                              ; only '-' in input

.CheckDigit:
    mov bl, byte [rsi]
    sub bl, '0'
    cmp bl, 9
    ja .ExitError                                  ; not a digit

.ConvertLoop:
    mov bl, byte [rsi]
    sub bl, '0'                                ; convert to digit
    imul rax, r10                              ; rax *= 10
    add rax, rbx                               ; rax += digit

    inc rsi                                    ; next char
    dec rcx
    jnz .ConvertLoop

;;; apply sign if needed
    test rdi, rdi
    jz .ExitSuccess
    neg rax

.ExitSuccess:
    ret

.ExitError:
    xor rax, rax                               ; return 0 if error
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
    cmp rdx, 10
    jb .below_10
    add rdx, 'A' - 10
    jmp .push_char
    .below_10:
    add rdx, '0'
    .push_char:
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

