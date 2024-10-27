#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <driver/ata_def.h>

struct ata_device_list {
    bool primary_master;
    bool primary_slave;
    bool secondary_master;
    bool secondary_slave;
};
extern struct ata_device_list ata_device_list;

extern uint8_t* ide_buffer;

void ide_select_drive(uint8_t bus, uint8_t i);
void init_ata();

uint8_t ide_identify(uint8_t bus, uint8_t drive);
uint8_t ata_read_one(uint8_t *buf, uint32_t lba, uint8_t index);
bool ata_read(uint8_t *buf, uint32_t lba, uint32_t numsects, uint8_t index);
