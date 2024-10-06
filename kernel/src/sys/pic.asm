section .data
PIC1_COMMAND_PORT equ 0x20
PIC1_DATA_PORT    equ 0x21
PIC2_COMMAND_PORT equ 0xA0
PIC2_DATA_PORT    equ 0xA1

PIC_ICW1_ICW4     equ 0x01
PIC_ICW1_INIT     equ 0x10
PIC_ICW4_8086     equ 0x01
PIC_CMD_EOI       equ 0x20
PIC_CMD_READ_IRR  equ 0x0A
PIC_CMD_READ_ISR  equ 0x0B

section .text
global pic_init
global pic_sendeoi
global pic_disable
global pic_mask
global pic_unmask
global pic_read_irqrr
global pic_read_isr

pic_init:
    push eax
    push ecx
    push edx

    mov al, PIC_ICW1_ICW4 | PIC_ICW1_INIT
    out PIC1_COMMAND_PORT, al
    call io_wait

    out PIC2_COMMAND_PORT, al
    call io_wait

    mov al, cl
    out PIC1_DATA_PORT, al
    call io_wait

    mov al, dl
    out PIC2_DATA_PORT, al
    call io_wait

    mov al, 0x04
    out PIC1_DATA_PORT, al
    call io_wait

    mov al, 0x02
    out PIC2_DATA_PORT, al
    call io_wait

    mov al, PIC_ICW4_8086
    out PIC1_DATA_PORT, al
    call io_wait
    out PIC2_DATA_PORT, al
    call io_wait

    mov al, 0
    out PIC1_DATA_PORT, al
    call io_wait
    out PIC2_DATA_PORT, al
    call io_wait

    pop edx
    pop ecx
    pop eax
    ret

pic_sendeoi:
    push eax

    cmp edi, 8
    jl .send_eoi_pic1

    mov al, PIC_CMD_EOI
    out PIC2_COMMAND_PORT, al

.send_eoi_pic1:
    mov al, PIC_CMD_EOI
    out PIC1_COMMAND_PORT, al

    pop eax
    ret

pic_disable:
    push eax
    mov al, 0xFF
    out PIC1_DATA_PORT, al
    call io_wait
    out PIC2_DATA_PORT, al
    call io_wait
    pop eax
    ret

pic_mask:
    push eax
    push ecx

    cmp edi, 8
    jle .pic1_mask

    sub edi, 8

.pic1_mask:
    in al, PIC1_DATA_PORT
    mov cl, 1

    ; Shift `cl` left by `edi` bits, one bit at a time
    mov ecx, edi
.loop_shl:
    shl cl, 1
    dec ecx
    jnz .loop_shl

    or al, cl
    out PIC1_DATA_PORT, al

    pop ecx
    pop eax
    ret

pic_unmask:
    push eax
    push ecx

    cmp edi, 8
    jle .pic1_unmask

    sub edi, 8

.pic1_unmask:
    in al, PIC1_DATA_PORT
    mov cl, 1

    ; Shift `cl` left by `edi` bits, one bit at a time
    mov ecx, edi
.loop_unmask_shl:
    shl cl, 1
    dec ecx
    jnz .loop_unmask_shl

    not cl
    and al, cl
    out PIC1_DATA_PORT, al

    pop ecx
    pop eax
    ret

pic_read_irqrr:
    push eax
    push edx

    mov al, PIC_CMD_READ_IRR
    out PIC1_COMMAND_PORT, al
    in al, PIC1_COMMAND_PORT
    movzx eax, al

    mov al, PIC_CMD_READ_IRR
    out PIC2_COMMAND_PORT, al
    in al, PIC2_COMMAND_PORT
    movzx edx, al
    mov ecx, 8
.loop_shl_irqrr:
    shl edx, 1
    dec ecx
    jnz .loop_shl_irqrr
    or eax, edx

    pop edx
    pop eax
    ret

pic_read_isr:
    push eax
    push edx

    mov al, PIC_CMD_READ_ISR
    out PIC1_COMMAND_PORT, al
    in al, PIC1_COMMAND_PORT
    movzx eax, al

    mov al, PIC_CMD_READ_ISR
    out PIC2_COMMAND_PORT, al
    in al, PIC2_COMMAND_PORT
    movzx edx, al
    mov ecx, 8
.loop_shl_isr:
    shl edx, 1
    dec ecx
    jnz .loop_shl_isr
    or eax, edx

    pop edx
    pop eax
    ret

io_wait:
    out 0x80, al
    ret
