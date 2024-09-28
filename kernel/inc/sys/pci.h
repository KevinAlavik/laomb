#pragma once

#include <stdint.h>
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

#define PCI_DEV_ENABLE_SHIFT   31
#define PCI_DEV_RESERVED_SHIFT 24
#define PCI_DEV_BUS_NUM_SHIFT  16
#define PCI_DEV_DEVICE_NUM_SHIFT 11
#define PCI_DEV_FUNCTION_NUM_SHIFT 8
#define PCI_DEV_FIELD_NUM_SHIFT 2
#define PCI_DEV_ALWAYS_ZERO_SHIFT 0

#define PCI_DEV_ENABLE_MASK    (1U << PCI_DEV_ENABLE_SHIFT)
#define PCI_DEV_RESERVED_MASK  (0x7FU << PCI_DEV_RESERVED_SHIFT)
#define PCI_DEV_BUS_NUM_MASK   (0xFFU << PCI_DEV_BUS_NUM_SHIFT)
#define PCI_DEV_DEVICE_NUM_MASK (0x1FU << PCI_DEV_DEVICE_NUM_SHIFT)
#define PCI_DEV_FUNCTION_NUM_MASK (0x7U << PCI_DEV_FUNCTION_NUM_SHIFT)
#define PCI_DEV_FIELD_NUM_MASK (0x3FU << PCI_DEV_FIELD_NUM_SHIFT)
#define PCI_DEV_ALWAYS_ZERO_MASK (0x3U << PCI_DEV_ALWAYS_ZERO_SHIFT)

typedef uint32_t pci_dev_t;

#define SET_PCI_DEV_ENABLE(bits, val) ((bits) = ((bits) & ~PCI_DEV_ENABLE_MASK) | ((val) << PCI_DEV_ENABLE_SHIFT))
#define SET_PCI_DEV_RESERVED(bits, val) ((bits) = ((bits) & ~PCI_DEV_RESERVED_MASK) | ((val) << PCI_DEV_RESERVED_SHIFT))
#define SET_PCI_DEV_BUS_NUM(bits, val) ((bits) = ((bits) & ~PCI_DEV_BUS_NUM_MASK) | ((val) << PCI_DEV_BUS_NUM_SHIFT))
#define SET_PCI_DEV_DEVICE_NUM(bits, val) ((bits) = ((bits) & ~PCI_DEV_DEVICE_NUM_MASK) | ((val) << PCI_DEV_DEVICE_NUM_SHIFT))
#define SET_PCI_DEV_FUNCTION_NUM(bits, val) ((bits) = ((bits) & ~PCI_DEV_FUNCTION_NUM_MASK) | ((val) << PCI_DEV_FUNCTION_NUM_SHIFT))
#define SET_PCI_DEV_FIELD_NUM(bits, val) ((bits) = ((bits) & ~PCI_DEV_FIELD_NUM_MASK) | ((val) << PCI_DEV_FIELD_NUM_SHIFT))
#define SET_PCI_DEV_ALWAYS_ZERO(bits, val) ((bits) = ((bits) & ~PCI_DEV_ALWAYS_ZERO_MASK) | ((val) << PCI_DEV_ALWAYS_ZERO_SHIFT))

#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0a
#define PCI_CLASS                0x0b
#define PCI_CACHE_LINE_SIZE      0x0c
#define PCI_LATENCY_TIMER        0x0d
#define PCI_HEADER_TYPE          0x0e
#define PCI_BIST                 0x0f
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_SECONDARY_BUS        0x09

#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106
#define PCI_NONE 0xFFFF

#define MAX_DEVICE_PER_BUS       32
#define DEVICE_PER_BUS           32
#define FUNCTION_PER_DEVICE      32


uint32_t pci_read(pci_dev_t dev, uint32_t field);
void pci_write(pci_dev_t dev, uint32_t field, uintptr_t val);
uint32_t get_device_type(pci_dev_t dev);
uint32_t get_secondary_bus(pci_dev_t dev);
uint32_t pci_reach_end(pci_dev_t dev);

pci_dev_t pci_scan_function(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, uint32_t function, int device_type);
pci_dev_t pci_scan_device(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, int device_type);
pci_dev_t pci_scan_bus(uint16_t vendor_id, uint16_t device_id, uint32_t bus, int device_type);
pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type);
void pci_init();