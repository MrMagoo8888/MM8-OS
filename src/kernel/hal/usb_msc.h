#ifndef USB_MSC_H
#define USB_MSC_H

#include "stdint.h"

// Command Block Wrapper (CBW) - Sent to the device
typedef struct {
    uint32_t signature;          // 'USBC' (0x43425355)
    uint32_t tag;                // Unique ID for the command
    uint32_t transfer_length;    // Bytes to transfer
    uint8_t  flags;             // Bit 7: 0=Out, 1=In
    uint8_t  lun;               // Logical Unit Number
    uint8_t  command_length;     // Length of SCSI command (1-16)
    uint8_t  scsi_command[16];   // The actual SCSI CDB
} __attribute__((packed)) usb_msc_cbw_t;

// Command Status Wrapper (CSW) - Received from the device
typedef struct {
    uint32_t signature;          // 'USBS' (0x53425355)
    uint32_t tag;                // Must match the CBW tag
    uint32_t data_residue;       // Bytes not transferred
    uint8_t  status;            // 0=Success, 1=Failed, 2=Phase Error
} __attribute__((packed)) usb_msc_csw_t;

#define MSC_CBW_SIGNATURE 0x43425355
#define MSC_CSW_SIGNATURE 0x53425355

// SCSI Commands (Common ones used in BOT)
#define SCSI_READ_10       0x28
#define SCSI_WRITE_10      0x2A
#define SCSI_READ_CAPACITY 0x25
#define SCSI_INQUIRY       0x12

#endif // USB_MSC_H