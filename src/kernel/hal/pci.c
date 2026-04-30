#include "pci.h"
#include <arch/i686/io.h>
#include "stdio.h"

// These functions assume i686_outl and i686_inl exist in io.h/asm
uint32_t pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    i686_outl(0xCF8, address);
    return i686_inl(0xCFC);
}

void pci_write_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    i686_outl(0xCF8, address);
    i686_outl(0xCFC, value);
}

extern void ohci_init(pci_device_t* dev);

void pci_init_device(pci_device_t* dev) {
    // USB Class 0x0C, Subclass 0x03
    if (dev->class_id == 0x0C && dev->subclass_id == 0x03) {
        if (dev->prog_if == 0x10) {
            printf("PCI: Found OHCI USB Controller at %02x:%02x.%d\n", dev->bus, dev->device, dev->function);
            ohci_init(dev);
        }
    }
}

void pci_enumerate() {
    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t slot = 0; slot < 32; slot++) {
            for (uint32_t func = 0; func < 8; func++) {
                uint32_t vendor_device = pci_read_config(bus, slot, func, 0);
                uint16_t vendor = vendor_device & 0xFFFF;
                if (vendor == 0xFFFF) continue;

                uint32_t class_rev = pci_read_config(bus, slot, func, 0x08);
                
                pci_device_t dev;
                dev.bus = (uint8_t)bus;
                dev.device = (uint8_t)slot;
                dev.function = (uint8_t)func;
                dev.vendor_id = vendor;
                dev.device_id = (uint16_t)(vendor_device >> 16);
                dev.class_id = (uint8_t)(class_rev >> 24);
                dev.subclass_id = (uint8_t)(class_rev >> 16);
                dev.prog_if = (uint8_t)(class_rev >> 8);

                pci_init_device(&dev);

                // If not a multi-function device, don't check other functions
                if (func == 0) {
                    uint32_t header_type = pci_read_config(bus, slot, 0, 0x0C);
                    if (!(header_type & 0x00800000)) break;
                }
            }
        }
    }
}