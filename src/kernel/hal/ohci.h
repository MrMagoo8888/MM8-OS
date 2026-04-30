#pragma once
#include "stdint.h"
#include "pci.h"

// OHCI Register Offsets
#define OHCI_REG_REVISION           0x00
#define OHCI_REG_CONTROL            0x04
#define OHCI_REG_COMMAND_STATUS     0x08
#define OHCI_REG_INTERRUPT_STATUS   0x0C
#define OHCI_REG_INTERRUPT_ENABLE   0x10
#define OHCI_REG_HCCA               0x18
#define OHCI_REG_CONTROL_HEAD_ED    0x20
#define OHCI_REG_BULK_HEAD_ED       0x28
#define OHCI_REG_RH_DESCRIPTOR_A    0x48
#define OHCI_REG_RH_PORT_STATUS     0x54

// HcControl Bits
#define OHCI_CONTROL_HCFS_OPERATIONAL (1 << 7)

// HcCommandStatus Bits
#define OHCI_CMD_STATUS_HCR           (1 << 0)

// HCCA Structure (Must be 256-byte aligned)
typedef struct {
    uint32_t interrupt_table[32];
    uint16_t frame_number;
    uint16_t pad1;
    uint32_t done_head;
    uint8_t  reserved[116];
} __attribute__((packed, aligned(256))) ohci_hcca_t;

typedef struct {
    uint32_t mmio_base;
    ohci_hcca_t* hcca;
    uint8_t num_ports;
} ohci_controller_t;

void ohci_init(pci_device_t* dev);
void ohci_write_reg(ohci_controller_t* hc, uint32_t reg, uint32_t val);
uint32_t ohci_read_reg(ohci_controller_t* hc, uint32_t reg);