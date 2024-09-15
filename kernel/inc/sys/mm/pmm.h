#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>
#include <ultra_protocol.h>

void pmm_init(struct ultra_boot_context* ctx);
void* pmm_alloc();
void pmm_free(void* ptr);

#endif
