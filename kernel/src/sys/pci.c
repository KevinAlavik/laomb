#include <sys/pci.h>
#include <kprintf>
#include <kheap.h>
#include <io.h>

uint32_t pci_size_map[100];
pci_dev_t dev_zero = {0};

uint32_t pci_read(pci_dev_t dev, uint32_t field) {
    SET_PCI_DEV_FIELD_NUM(dev, (field & 0xFC) >> 2);
    SET_PCI_DEV_ENABLE(dev, 1);
	outl(PCI_CONFIG_ADDRESS, dev);

	uint32_t size = pci_size_map[field];
	if (size == 1) {
		uint8_t t = inb(PCI_CONFIG_DATA + (field & 3));
		return t;
	}
	else if (size == 2) {
		uint16_t t = inw(PCI_CONFIG_DATA + (field & 2));
		return t;
	}
	else if (size == 4){
		uint32_t t = inl(PCI_CONFIG_DATA);
		return t;
	}
	return 0xffff;
}

void pci_write(pci_dev_t dev, uint32_t field, uint32_t value) {
    SET_PCI_DEV_FIELD_NUM(dev, (field & 0xFC) >> 2);
    SET_PCI_DEV_ENABLE(dev, 1);
	outl(PCI_CONFIG_ADDRESS, dev);
	outl(PCI_CONFIG_DATA, value);
}

uint32_t get_device_type(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_CLASS) << 8;
	return t | pci_read(dev, PCI_SUBCLASS);
}

uint32_t get_secondary_bus(pci_dev_t dev) {
	return pci_read(dev, PCI_SECONDARY_BUS);
}

uint32_t pci_reach_end(pci_dev_t dev) {
	uint32_t t = pci_read(dev, PCI_HEADER_TYPE);
	return !t;
}

pci_dev_t pci_scan_function(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, uint32_t function, int device_type) {
	pci_dev_t dev = {0};
    SET_PCI_DEV_BUS_NUM(dev, bus);
    SET_PCI_DEV_DEVICE_NUM(dev, device);
    SET_PCI_DEV_FUNCTION_NUM(dev, function);
	if (get_device_type(dev) == PCI_TYPE_BRIDGE) {
		pci_scan_bus(vendor_id, device_id, get_secondary_bus(dev), device_type);
	}
	if (device_type == -1 || device_type == (int)get_device_type(dev)) {
		uint32_t devid  = pci_read(dev, PCI_DEVICE_ID);
		uint32_t vendid = pci_read(dev, PCI_VENDOR_ID);
		if (devid == device_id && vendor_id == vendid) return dev;
	}
	return dev_zero;
}

pci_dev_t pci_scan_device(uint16_t vendor_id, uint16_t device_id, uint32_t bus, uint32_t device, int device_type) {
	pci_dev_t dev = {0};
    SET_PCI_DEV_BUS_NUM(dev, bus);
    SET_PCI_DEV_DEVICE_NUM(dev, device);

	if (pci_read(dev,PCI_VENDOR_ID) == PCI_NONE) return dev_zero;

	pci_dev_t t = pci_scan_function(vendor_id, device_id, bus, device, 0, device_type);
	if (t) return t;

	if (pci_reach_end(dev)) return dev_zero;

	for (int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		if (pci_read(dev,PCI_VENDOR_ID) != PCI_NONE) {
			t = pci_scan_function(vendor_id, device_id, bus, device, function, device_type);
			if (t) return t;
		}
	}
	return dev_zero;
}

pci_dev_t pci_scan_bus(uint16_t vendor_id, uint16_t device_id, uint32_t bus, int device_type) {
	for (int device = 0; device < DEVICE_PER_BUS; device++) {
		pci_dev_t t = pci_scan_device(vendor_id, device_id, bus, device, device_type);
		if(t) return t;
	}
	return dev_zero;
}

pci_dev_t pci_get_device(uint16_t vendor_id, uint16_t device_id, int device_type) {

	pci_dev_t t = pci_scan_bus(vendor_id, device_id, 0, device_type);
	if (t) return t;

	// Handle multiple pci host controllers

	if (pci_reach_end(dev_zero)) {
		kprintf("PCI Get device failed...\n");
	}
	for (int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		pci_dev_t dev = {0};
        SET_PCI_DEV_FUNCTION_NUM(dev, function);

		if (pci_read(dev, PCI_VENDOR_ID) == PCI_NONE) break;
		t = pci_scan_bus(vendor_id, device_id, function, device_type);
		if (t) return t;
	}
	return dev_zero;
}

void pci_init() {
	pci_size_map[PCI_VENDOR_ID] =	2;
	pci_size_map[PCI_DEVICE_ID] =	2;
	pci_size_map[PCI_COMMAND]	=	2;
	pci_size_map[PCI_STATUS]	=	2;
	pci_size_map[PCI_SUBCLASS]	=	1;
	pci_size_map[PCI_CLASS]		=	1;
	pci_size_map[PCI_CACHE_LINE_SIZE]	= 1;
	pci_size_map[PCI_LATENCY_TIMER]		= 1;
	pci_size_map[PCI_HEADER_TYPE] = 1;
	pci_size_map[PCI_BIST] = 1;
	pci_size_map[PCI_BAR0] = 4;
	pci_size_map[PCI_BAR1] = 4;
	pci_size_map[PCI_BAR2] = 4;
	pci_size_map[PCI_BAR3] = 4;
	pci_size_map[PCI_BAR4] = 4;
	pci_size_map[PCI_BAR5] = 4;
	pci_size_map[PCI_INTERRUPT_LINE]	= 1;
	pci_size_map[PCI_SECONDARY_BUS]		= 1;
}