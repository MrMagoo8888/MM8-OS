#include "stdint.h"
#include "arch/i686/io.h"
#include "stdio.h"
#include "ehci.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                       ((uint32_t)func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000);
    i686_outl(PCI_CONFIG_ADDRESS, address);
    return i686_inl(PCI_CONFIG_DATA);
}

void pci_enumerate() {
    printf("PCI: Scanning bus for USB controllers...\n");
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor_device = pci_read_config(bus, slot, func, 0);
                if ((vendor_device & 0xFFFF) == 0xFFFF) continue;

                uint32_t class_reg = pci_read_config(bus, slot, func, 0x08);
                uint8_t class_code = (class_reg >> 24) & 0xFF;
                uint8_t subclass   = (class_reg >> 16) & 0xFF;
                uint8_t prog_if    = (class_reg >> 8)  & 0xFF;

                // Class 0x0C is Serial Bus, Subclass 0x03 is USB
                if (class_code == 0x0C && subclass == 0x03) {
                    const char* type = "Unknown USB";
                    uint32_t bar0 = pci_read_config(bus, slot, func, 0x10);

                    if (prog_if == 0x00) type = "UHCI";
                    else if (prog_if == 0x10) type = "OHCI";
                    else if (prog_if == 0x20) type = "EHCI (USB 2.0)";
                    else if (prog_if == 0x30) type = "XHCI (USB 3.0)";

                    printf("PCI: Found %s at %d:%d:%d - BAR0: %x\n", type, bus, slot, func, bar0);

                    if (prog_if == 0x20) { // EHCI
                        ehci_init(bar0);
                    }
                }
            }
        }
    }
}