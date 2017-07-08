USE64
mov r9, r8
mov r8, rcx
mov rcx, rdx
mov rdx, rsi 
mov rsi, rdi
mov rdi, 0x0123456789abcdef


push r9
; r12 is callee-saved, so save it before using
push r12
lea r12, [rsp + 0x01234]
lea rax, [rsp + 8]
mov r11, [rsp + 8]
cond:
    cmp r12, rax
    je init2
; move former 6th arg
loop:
    mov r10, [rax + 8]
    mov [rax], r10
    add rax, 8
    jmp cond
init2:
    mov [r12], r11
    mov r11, [rsp + 8]
    lea r12, [rsp + 0xabcdef]
    lea rax, [rsp + 8]
cond2:
    cmp r12, rax
    je exit
; move return address
loop2:
    mov r10, [rax + 8]
    mov [rax], r10
    add rax, 8
    jmp cond2
exit:
    mov [r12], r11
    pop r12

; return stack to initial state
rev2:
    mov r11, [rsp + 0xabcdef - 8]
    lea rax, [rsp + 0xabcdef - 8]
rcond2:
    cmp rax, rsp
    je rev
; move return address to the top of stack
rloop2:
    mov r10, [rax - 8]
    mov [rax], r10
    sub rax, 8
    jmp rcond2
rev:
    mov [rsp], r11
    lea rax, [rsp + 0x01234 - 8]
rcond:
    cmp rax, rsp
    je final
; move SSE args that are before new int arg to stack's bottom, erasing int arg
rloop:
    mov r10, [rax - 8]
    mov [rax], r10
    sub rax, 8
    jmp rcond
final:
    pop r10
    ret