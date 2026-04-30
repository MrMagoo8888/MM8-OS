#pragma once
#include "stdint.h"

typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_id;
    uint8_t subclass_id;
    uint8_t prog_if;
} pci_device_t;

uint32_t pci_read_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset);
void pci_write_config(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t value);
void pci_enumerate();
void pci_init_device(pci_device_t* dev);