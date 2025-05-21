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
sub rsp, 0 ; rsp = rbp - local_vars_cnt
push rax ; ret val
push rbx ; old rbp
push 228
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

