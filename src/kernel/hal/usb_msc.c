#include "usb_msc.h"
#include "stdio.h"
#include "memory.h"
#include "ehci.h"

extern int usb_init_device_msc(ehci_controller_t* hc);

// This is a high-level skeleton. In a real driver, you'd call
// your EHCI/OHCI bulk transfer functions here.
int usb_msc_perform_bot(void* device, uint8_t endpoint_out, uint8_t endpoint_in,
                        void* scsi_cmd, uint8_t scsi_len, 
                        void* data, uint32_t data_len, int is_read) {
    
    static uint32_t tag_sequence = 0x12345678;
    uint32_t current_tag = tag_sequence++;

    usb_msc_cbw_t* cbw = (usb_msc_cbw_t*)malloc_aligned(sizeof(usb_msc_cbw_t), 32);
    usb_msc_csw_t* csw = (usb_msc_csw_t*)malloc_aligned(sizeof(usb_msc_csw_t), 32);
    
    if (!cbw || !csw) return -1;

    ehci_controller_t* hc = (ehci_controller_t*)device;
    static bool initialized = false;
    uint8_t addr = 1;

    // From Scratch: We must initialize the device state before BOT works
    if (!initialized) {
        if (usb_init_device_msc(hc) == 0) {
            initialized = true;
        } else {
            free_aligned(cbw); free_aligned(csw);
            return -1;
        }
    }

    // Stage 1: Command Block Wrapper

    printf("Starting Command Block Wrapper Phase:\n");
    memset(cbw, 0, sizeof(usb_msc_cbw_t));
    cbw->signature = MSC_CBW_SIGNATURE;
    cbw->tag = current_tag;
    cbw->transfer_length = data_len;
    cbw->flags = (is_read) ? 0x80 : 0x00;
    cbw->lun = 0;
    cbw->command_length = scsi_len;
    memcpy(cbw->scsi_command, scsi_cmd, scsi_len);

    printf("CBW Sent\n");
    if (ehci_bulk_transfer(hc, addr, endpoint_out, cbw, 31, 0) != 0) {
        printf("BOT Error: CBW Phase failed\n");
        goto error_out;
    }

    // Stage 2: Data Phase
    printf("BOT: Starting Data Phase\n");
    if (data_len > 0) {
        if (ehci_bulk_transfer(hc, addr, (is_read ? endpoint_in : endpoint_out), data, data_len, is_read) != 0) {
            printf("BOT Error: Data Phase failed\n");
            // Note: If data fails, we still try to read CSW to "clear" the pipe
        }
    }

    // Stage 3: Command Status Wrapper
    printf("BOT: Starting CSW Phase\n");
    memset(csw, 0, sizeof(usb_msc_csw_t));
    if (ehci_bulk_transfer(hc, addr, endpoint_in, csw, 13, 1) != 0) {
        printf("BOT Error: CSW Phase failed (No response)\n");
        goto error_out;
    }

    // Verify integrity
    printf("BOT: Verifying CSW\n");
    if (csw->signature != MSC_CSW_SIGNATURE) {
        printf("BOT Error: CSW Invalid Signature %x\n", csw->signature);
        goto error_out;
    }

    printf("BOT: CSW Verified\n");
    if (csw->tag != current_tag) {
        printf("BOT Error: CSW Tag Mismatch\n");
        goto error_out;
    }

    printf("BOT: Checking CSW Status\n");
    if (csw->status != 0) {
        printf("BOT: SCSI Command Failed (Status %d)\n", csw->status);
        goto error_out;
    }

    free_aligned(cbw);
    free_aligned(csw);
    return 0;

error_out:
    free_aligned(cbw);
    free_aligned(csw);
    return -1;
}

int usb_msc_read_sectors(ehci_controller_t* hc, uint32_t lba, uint8_t count, void* buffer) {
    uint8_t cmd[10] = {0};
    cmd[0] = SCSI_READ_10;
    
    // LBA (Big Endian conversion)
    cmd[2] = (lba >> 24) & 0xFF;
    cmd[3] = (lba >> 16) & 0xFF;
    cmd[4] = (lba >> 8) & 0xFF;
    cmd[5] = lba & 0xFF;
    
    // Transfer length in sectors (Big Endian)
    cmd[7] = 0; 
    cmd[8] = count;

    // Common QEMU endpoints are 1 for OUT and 2 (or 0x81) for IN
    // In a full driver, you should parse descriptors to get these
    return usb_msc_perform_bot(hc, 1, 2, cmd, 10, buffer, count * 512, 1);
}

int usb_msc_write_sectors(ehci_controller_t* hc, uint32_t lba, uint8_t count, const void* buffer) {
    uint8_t cmd[10] = {0};
    cmd[0] = SCSI_WRITE_10;
    
    // LBA (Big Endian)
    cmd[2] = (lba >> 24) & 0xFF;
    cmd[3] = (lba >> 16) & 0xFF;
    cmd[4] = (lba >> 8) & 0xFF;
    cmd[5] = lba & 0xFF;
    
    // Transfer length (Big Endian)
    cmd[7] = 0; 
    cmd[8] = count;

    return usb_msc_perform_bot(hc, 1, 2, cmd, 10, (void*)buffer, count * 512, 0);
}