#include <sys/pic.h>
#include <io.h>

enum {
    PIC_ICW1_ICW4           = 0x01,
    PIC_ICW1_SINGLE         = 0x02,
    PIC_ICW1_INTERVAL4      = 0x04,
    PIC_ICW1_LEVEL          = 0x08,
    PIC_ICW1_INITIALIZE     = 0x10
} PIC_ICW1;

enum {
    PIC_ICW4_8086           = 0x1,
    PIC_ICW4_AUTO_EOI       = 0x2,
    PIC_ICW4_BUFFER_MASTER  = 0x4,
    PIC_ICW4_BUFFER_SLAVE   = 0x0,
    PIC_ICW4_BUFFERRED      = 0x8,
    PIC_ICW4_SFNM           = 0x10,
} PIC_ICW4;


enum {
    PIC_CMD_END_OF_INTERRUPT    = 0x20,
    PIC_CMD_READ_IRR            = 0x0A,
    PIC_CMD_READ_ISR            = 0x0B,
} PIC_CMD;

void pic_init(uint8_t offsetPic1, uint8_t offsetPic2)
{
    outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();
    outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    io_wait();

    outb(PIC1_DATA_PORT, offsetPic1);
    io_wait();
    outb(PIC2_DATA_PORT, offsetPic2);
    io_wait();

    outb(PIC1_DATA_PORT, 0x4);
    io_wait();
    outb(PIC2_DATA_PORT, 0x2);
    io_wait();

    outb(PIC1_DATA_PORT, PIC_ICW4_8086);
    io_wait();
    outb(PIC2_DATA_PORT, PIC_ICW4_8086);
    io_wait();

    outb(PIC1_DATA_PORT, 0);
    io_wait();
    outb(PIC2_DATA_PORT, 0);
    io_wait();
}

void pic_sendeoi(int irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void pic_disable()
{
    outb(PIC1_DATA_PORT, 0xFF);
    io_wait();
    outb(PIC2_DATA_PORT, 0xFF);
    io_wait();
}

void pic_mask(int irq)
{
    if (irq > 8) 
        irq -= 8;

    uint8_t mask = inb(PIC1_DATA_PORT);
    outb(PIC1_DATA_PORT,  mask | (1 << irq));
}

void pic_unmask(int irq)
{
    if (irq > 8)
        irq -= 8;

    uint8_t mask = inb(PIC1_DATA_PORT);
    outb(PIC1_DATA_PORT,  mask & ~(1 << irq));
}

uint16_t pic_read_irqrr()
{
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return ((uint16_t)inb(PIC2_COMMAND_PORT)) | (((uint16_t)inb(PIC2_COMMAND_PORT)) << 8);
}

uint16_t pic_read_isr()
{
    outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return ((uint16_t)inb(PIC2_COMMAND_PORT)) | (((uint16_t)inb(PIC2_COMMAND_PORT)) << 8);
}