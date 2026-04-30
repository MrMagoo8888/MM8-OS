#include "ohci.h"
#include "stdio.h"
#include "memory.h"
#include "time.h"

void ohci_write_reg(ohci_controller_t* hc, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(hc->mmio_base + reg) = val;
}

uint32_t ohci_read_reg(ohci_controller_t* hc, uint32_t reg) {
    return *(volatile uint32_t*)(hc->mmio_base + reg);
}

void ohci_init(pci_device_t* dev) {
    // Get MMIO Base from BAR0
    uint32_t bar0 = pci_read_config(dev->bus, dev->device, dev->function, 0x10);
    if (bar0 & 1) { // We expect MMIO, not I/O ports
        printf("OHCI: Error - BAR0 is I/O, expected MMIO\n");
        return;
    }
    
    ohci_controller_t hc;
    hc.mmio_base = bar0 & 0xFFFFFFF0;

    // 1. Enable PCI Bus Mastering and Memory Space
    uint32_t command = pci_read_config(dev->bus, dev->device, dev->function, 0x04);
    pci_write_config(dev->bus, dev->device, dev->function, 0x04, command | 0x06);

    // 2. Reset Controller
    ohci_write_reg(&hc, OHCI_REG_COMMAND_STATUS, OHCI_CMD_STATUS_HCR);
    sleep_ms(10);

    // 3. Setup HCCA (Host Controller Communications Area)
    hc.hcca = (ohci_hcca_t*)malloc_aligned(sizeof(ohci_hcca_t), 256);
    memset(hc.hcca, 0, sizeof(ohci_hcca_t));
    ohci_write_reg(&hc, OHCI_REG_HCCA, (uint32_t)hc.hcca);

    // 4. Initialize List Heads
    ohci_write_reg(&hc, OHCI_REG_CONTROL_HEAD_ED, 0);
    ohci_write_reg(&hc, OHCI_REG_BULK_HEAD_ED, 0);

    // 5. Set Operational State
    uint32_t control = ohci_read_reg(&hc, OHCI_REG_CONTROL);
    control &= ~(3 << 6); // Clear HCFS bits
    control |= OHCI_CONTROL_HCFS_OPERATIONAL;
    ohci_write_reg(&hc, OHCI_REG_CONTROL, control);

    // 6. Get number of ports
    uint32_t rh_a = ohci_read_reg(&hc, OHCI_REG_RH_DESCRIPTOR_A);
    hc.num_ports = (uint8_t)(rh_a & 0xFF);

    printf("OHCI: Controller initialized. Ports: %d\n", hc.num_ports);
    
    // Power up ports
    for (int i = 0; i < hc.num_ports; i++) {
        ohci_write_reg(&hc, OHCI_REG_RH_PORT_STATUS + (i * 4), 0x00000100); // SetPortPower
    }
}