#pragma once
#include "stdint.h"
#include "pci.h"

// EHCI Capability Registers Offsets
#define EHCI_CAP_CAPLENGTH          0x00
#define EHCI_CAP_HCIVERSION         0x02
#define EHCI_CAP_HCSPARAMS          0x04
#define EHCI_CAP_HCCPARAMS          0x08

// EHCI Operational Registers Offsets (relative to OpBase)
#define EHCI_OP_USBCMD              0x00
#define EHCI_OP_USBSTS              0x04
#define EHCI_OP_USBINTR             0x08
#define EHCI_OP_FRINDEX             0x0C
#define EHCI_OP_CTRLDSSEGMENT       0x10
#define EHCI_OP_PERIODICLISTBASE    0x14
#define EHCI_OP_ASYNCLISTADDR       0x18
#define EHCI_OP_CONFIGFLAG          0x40
#define EHCI_OP_PORTSC              0x44

// Bit definitions
#define EHCI_CMD_RUN                (1 << 0)   // Run/Stop
#define EHCI_CMD_HCRESET            (1 << 1)   // Host Controller Reset
#define EHCI_CMD_ASYNCE             (1 << 5)   // Async Schedule Enable

#define EHCI_STS_ASYNCS             (1 << 15)  // Async Schedule Status
#define EHCI_STS_HALTED             (1 << 12)

// qTD Token bits
#define QTD_ACTIVE                  (1 << 7)
#define QTD_IOC                     (1 << 15)  // Interrupt On Complete
#define QTD_PID_OUT                 0
#define QTD_PID_IN                  1
#define QTD_PID_SETUP               2

// QH Endpoint Characteristics
#define QH_DTC                      (1 << 14)  // Data Toggle Control
#define QH_H                        (1 << 15)  // Head of reclamation list
#define QH_SPEED_HIGH               (2 << 12)

// USB Standard Request Types
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_SET_CONFIGURATION   0x09

#define USB_DESC_DEVICE             0x01
#define USB_DESC_CONFIGURATION      0x02

// EHCI Queue Element Transfer Descriptor (qTD)
typedef struct {
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed, aligned(32))) ehci_qtd_t;

// EHCI Queue Head (QH)
typedef struct {
    uint32_t horizontal_link;
    uint32_t endpoint_char;
    uint32_t endpoint_caps;
    uint32_t current_qtd;
    // Transfer Overlay
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed, aligned(32))) ehci_qh_t;

typedef struct {
    uint32_t cap_base;
    uint32_t op_base;
    uint8_t num_ports;
    pci_device_t pci_dev;
    ehci_qh_t* async_qh; // The primary head of our async list
} ehci_controller_t;

void ehci_init(pci_device_t* dev);
int ehci_bulk_transfer(ehci_controller_t* hc, uint8_t addr, uint8_t ep, void* data, uint32_t len, int is_in);

typedef struct {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed)) usb_setup_packet_t;

int ehci_control_transfer(ehci_controller_t* hc, uint8_t addr, usb_setup_packet_t* setup, void* data);

static inline void ehci_write_op(ehci_controller_t* hc, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(hc->op_base + reg) = val;
}

static inline uint32_t ehci_read_op(ehci_controller_t* hc, uint32_t reg) {
    return *(volatile uint32_t*)(hc->op_base + reg);
}