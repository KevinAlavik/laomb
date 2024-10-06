bits 32
section .text

STACK_CHK_GUARD equ 0xe2dee396

puts:
    push ebp
    mov ebp, esp

    mov esi, [ebp + 8]
    .loop:
        movzx eax, byte [esi]
        cmp eax, 0
        je .end
        mov dx, 0xe9
        out dx, al
        inc esi
        jmp .loop
    .end:

    pop ebp
    ret

global __stack_chk_guard
__stack_chk_guard: dw STACK_CHK_GUARD
ssd_string: db "Stack smashing detected!", 0

global __stack_chk_fail
__stack_chk_fail:
    cli

    mov esi, ssd_string
    push esi
    call puts

@@1:
    hlt
    jmp @@1

global __stack_chk_fail_local
__stack_chk_fail_local:
    jmp __stack_chk_fail