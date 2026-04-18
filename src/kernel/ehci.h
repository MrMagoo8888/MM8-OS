#pragma once
#include "stdint.h"
#include "stdbool.h"

// EHCI Capability Registers (Offsets from BAR0)
#define EHCI_CAP_CAPLENGTH      0x00
#define EHCI_CAP_HCIVERSION     0x02
#define EHCI_CAP_HCSPARAMS      0x04
#define EHCI_CAP_HCCPARAMS      0x08

// EHCI Operational Registers (Offsets from BAR0 + CAPLENGTH)
#define EHCI_OP_USBCMD          0x00
#define EHCI_OP_USBSTS          0x04
#define EHCI_OP_USBINTR         0x08
#define EHCI_OP_FRINDEX         0x0C
#define EHCI_OP_CTRLDSSEGMENT   0x10
#define EHCI_OP_PERIODICLISTBASE 0x14
#define EHCI_OP_ASYNCLISTADDR   0x18
#define EHCI_OP_CONFIGFLAG      0x40
#define EHCI_OP_PORTSC          0x44

// USB Command Register Bits
#define EHCI_CMD_RUN            (1 << 0)
#define EHCI_CMD_HCRESET        (1 << 1)
#define EHCI_CMD_PERIODIC_EN    (1 << 4)
#define EHCI_CMD_ASYNC_EN       (1 << 5)

// USB Status Register Bits
#define EHCI_STS_HALTED         (1 << 12)

// Transfer Descriptor (qTD)
typedef struct {
    uint32_t next;
    uint32_t alt_next;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed, aligned(32))) EHCI_qTD;

// Queue Head (QH)
typedef struct {
    uint32_t horizontal_link;
    uint32_t capabilities[2];
    uint32_t current_qtd;
    EHCI_qTD qtd_overlay;
} __attribute__((packed, aligned(32))) EHCI_QH;

// SCSI Command Block Wrapper (CBW)
typedef struct {
    uint32_t signature;
    uint32_t tag;
    uint32_t transfer_len;
    uint8_t  flags;
    uint8_t  lun;
    uint8_t  cmd_len;
    uint8_t  scsi_cmd[16];
} __attribute__((packed)) USB_MSC_CBW;

typedef struct {
    uint32_t bar;
    uint32_t cap_reg_base;
    uint32_t op_reg_base;
    uint8_t  cap_length;
    uint16_t hci_version;
    uint32_t n_ports;
    uint8_t  device_addr;
    uint8_t  endpoint_in, endpoint_out;
} EHCI_Controller;

void ehci_init(uint32_t bar);
bool ehci_read_sectors(uint32_t lba, uint8_t count, void* buffer);
extern EHCI_Controller g_EhciController;