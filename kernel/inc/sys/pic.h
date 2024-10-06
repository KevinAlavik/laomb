#pragma once

#include <stdint.h>

#define PIC1_COMMAND_PORT           0x20
#define PIC1_DATA_PORT              0x21
#define PIC2_COMMAND_PORT           0xA0
#define PIC2_DATA_PORT              0xA1

void pic_init(uint8_t offsetPic1, uint8_t offsetPic2);
void pic_sendeoi(int irq);
void pic_disable();
void pic_mask(int irq);
void pic_unmask(int irq);
uint16_t pic_read_irqrr();
uint16_t pic_read_isr();