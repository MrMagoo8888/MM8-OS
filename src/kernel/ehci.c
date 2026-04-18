#include "ehci.h"
#include "stdio.h"
#include "memory.h"
#include "time.h"

// Missing EHCI Register Bit Definitions
#define EHCI_CMD_ASYNC_EN          0x0020  // Asynchronous Schedule Enable
#define EHCI_STS_ASYNC_OK          0x8000  // Asynchronous Schedule Status

// Helper to read from MMIO
static inline uint32_t mmio_read32(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

// Helper to write to MMIO
static inline void mmio_write32(uint32_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

EHCI_Controller g_EhciController;

// Simple Async List for transfers
static EHCI_QH* g_AsyncList = NULL;

// Helper to set up qTD buffer pointers (handling 4K page boundaries)
static void ehci_setup_qtd_buffer(EHCI_qTD* qtd, void* buffer, uint32_t len) {
    uint32_t addr = (uint32_t)buffer;
    qtd->buffer[0] = addr;
    for (int i = 1; i < 5; i++) {
        uint32_t page_boundary = (addr & ~0xFFF) + 0x1000;
        if (page_boundary < (uint32_t)buffer + len) {
            qtd->buffer[i] = page_boundary;
            addr = page_boundary;
        } else {
            qtd->buffer[i] = 0;
        }
    }
}

// Helper to wait for a transfer to complete
static bool ehci_wait_qtd(volatile EHCI_qTD* qtd) {
    uint32_t timeout = 1000000;
    while ((qtd->token & 0x80) && --timeout) { // Bit 7 is "Active"
        __asm__ volatile("pause");
    }

    // If we timed out or the Halted bit (bit 6) is set, the transfer failed
    if (timeout == 0 || (qtd->token & 0x40)) {
        return false;
    }

    // Check for errors (Bits 3-6: Halted, Data Buffer Error, Babble, Transaction Error)
    return (qtd->token & 0x7C) == 0;
}

static void ehci_init_async_list() {
    if (g_AsyncList) return;

    // Create the "Asynchronous Schedule" head. 
    // EHCI requires Queue Heads to be 32-byte aligned.
    g_AsyncList = (EHCI_QH*)malloc_aligned(sizeof(EHCI_QH), 32);
    if (!g_AsyncList) return;
    
    memset(g_AsyncList, 0, sizeof(EHCI_QH));

    // Link back to itself to create a circular list
    // 0x02 indicates this horizontal link points to another QH
    g_AsyncList->horizontal_link = ((uint32_t)g_AsyncList) | 0x02; 
    
    // Capabilities: Endpt Speed = High (2 << 12), Head (1 << 15), Max Packet = 512 (512 << 16)
    // Note: Production drivers must enumerate to find the correct Bulk Endpoints.
    g_AsyncList->capabilities[0] = (2 << 12) | (1 << 15) | (512 << 16);
    
    // Set the qTD overlay to "Terminate" (bit 0 = 1) and status to Halted (0x40)
    g_AsyncList->qtd_overlay.next = 1;
    g_AsyncList->qtd_overlay.alt_next = 1;
    g_AsyncList->qtd_overlay.token = 0x40; 

    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_ASYNCLISTADDR, (uint32_t)g_AsyncList);
}

bool ehci_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    if (!g_AsyncList) return false;

    // Detect if a device is actually on a port
    bool device_found = false;
    for (uint32_t i = 0; i < g_EhciController.n_ports; i++) {
        uint32_t port_addr = g_EhciController.op_reg_base + EHCI_OP_PORTSC + (i * 4);
        uint32_t portsc = mmio_read32(port_addr);
        
        // Check if device is connected (bit 0) AND port is enabled (bit 2)
        if ((portsc & 0x01) && (portsc & 0x04)) {
            device_found = true;
            break;
        }
    }

    if (!device_found) return false;

    // --- 0. Pause Async Schedule to safely modify descriptors ---
    uint32_t usbcmd = mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBCMD);
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBCMD, usbcmd & ~EHCI_CMD_ASYNC_EN);
    
    // Wait for schedule to stop
    int sched_timeout = 1000;
    while ((mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBSTS) & EHCI_STS_ASYNC_OK) && --sched_timeout) {
        __asm__ volatile("pause");
    }

    // Allocate DMA-safe buffers for the USB structures
    USB_MSC_CBW* cbw = (USB_MSC_CBW*)malloc_aligned(sizeof(USB_MSC_CBW), 32);
    uint8_t* csw_buffer = (uint8_t*)malloc_aligned(32, 32);
    memset(csw_buffer, 0, 32);

    // --- 1. Prepare SCSI READ(10) command ---
    memset(cbw, 0, sizeof(USB_MSC_CBW));
    cbw->signature = 0x43425355; // "USBC"
    cbw->tag = 0xDEADBEEF;
    cbw->transfer_len = count * 512;
    cbw->flags = 0x80; // Direction: Device-to-Host
    cbw->cmd_len = 10;
    
    cbw->scsi_cmd[0] = 0x28; // SCSI READ(10) Opcode
    cbw->scsi_cmd[2] = (lba >> 24) & 0xFF;
    cbw->scsi_cmd[3] = (lba >> 16) & 0xFF;
    cbw->scsi_cmd[4] = (lba >> 8) & 0xFF;
    cbw->scsi_cmd[5] = lba & 0xFF;
    cbw->scsi_cmd[8] = count;

    // --- 2. Build qTD chain (Command -> Data -> Status) ---
    EHCI_qTD* q_cmd = (EHCI_qTD*)malloc_aligned(sizeof(EHCI_qTD), 32);
    EHCI_qTD* q_data = (EHCI_qTD*)malloc_aligned(sizeof(EHCI_qTD), 32);
    EHCI_qTD* q_status = (EHCI_qTD*)malloc_aligned(sizeof(EHCI_qTD), 32);

    if (!q_cmd || !q_data || !q_status) return false;

    // q_cmd: Send CBW
    q_cmd->next = (uint32_t)q_data;
    q_cmd->alt_next = 1;
    q_cmd->token = (sizeof(USB_MSC_CBW) << 16) | (3 << 10) | (0 << 8) | 0x80; // PID=OUT, Err=3, Active
    ehci_setup_qtd_buffer(q_cmd, cbw, sizeof(USB_MSC_CBW));

    // q_data: Receive Sectors
    q_data->next = (uint32_t)q_status;
    q_data->alt_next = 1;
    q_data->token = ((count * 512) << 16) | (3 << 10) | (1 << 8) | 0x80; // PID=IN, Err=3, Active
    ehci_setup_qtd_buffer(q_data, buffer, count * 512);

    // q_status: Receive CSW
    q_status->next = 1; // Terminate chain
    q_status->alt_next = 1;
    q_status->token = (13 << 16) | (3 << 10) | (1 << 8) | 0x80; // PID=IN, Err=3, Active
    ehci_setup_qtd_buffer(q_status, csw_buffer, 13);

    // --- 3. Execute transfer ---
    // CRITICAL: We must zero out the current_qtd and clear the status bits in the overlay.
    // This forces the controller to re-fetch the 'next' qTD from memory.
    g_AsyncList->current_qtd = 0;
    g_AsyncList->qtd_overlay.next = (uint32_t)q_cmd;
    g_AsyncList->qtd_overlay.alt_next = 1;
    g_AsyncList->qtd_overlay.token = 0; // Clear Halted, Active, and Reset Toggle to 0

    // Re-enable Asynchronous Schedule
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBCMD, usbcmd | EHCI_CMD_ASYNC_EN);

    // Wait for the chain to complete
    bool success = ehci_wait_qtd(q_status);

    // Cleanup descriptors to prevent memory leaks
    free_aligned(q_cmd);
    free_aligned(q_data);
    free_aligned(q_status);
    free_aligned(cbw);
    free_aligned(csw_buffer);

    return success;
}

void ehci_init(uint32_t bar) {
    g_EhciController.bar = bar & 0xFFFFFFF0; // Mask out PCI flags
    
    // 1. Read Capability Registers
    g_EhciController.cap_reg_base = g_EhciController.bar;
    g_EhciController.cap_length = *(volatile uint8_t*)(g_EhciController.cap_reg_base + EHCI_CAP_CAPLENGTH);
    g_EhciController.hci_version = *(volatile uint16_t*)(g_EhciController.cap_reg_base + EHCI_CAP_HCIVERSION);
    
    uint32_t hcsparams = mmio_read32(g_EhciController.cap_reg_base + EHCI_CAP_HCSPARAMS);
    g_EhciController.n_ports = hcsparams & 0x0F;

    g_EhciController.op_reg_base = g_EhciController.bar + g_EhciController.cap_length;

    // 1.5 BIOS Handoff (OS Extended Capabilities)
    // This stops the BIOS from interfering with the controller.
    uint32_t hccparams = mmio_read32(g_EhciController.cap_reg_base + EHCI_CAP_HCCPARAMS);
    uint32_t eecp = (hccparams >> 8) & 0xFF;
    if (eecp >= 0x40) {
        uint32_t legsup = mmio_read32(g_EhciController.bar + eecp);
        if (legsup & 0x00010000) { // BIOS Semaphore bit
            mmio_write32(g_EhciController.bar + eecp, legsup | 0x01000000); // OS Semaphore
            int handoff_timeout = 100;
            while ((mmio_read32(g_EhciController.bar + eecp) & 0x00010000) && --handoff_timeout) {
                sleep_ms(10);
            }
        }
    }

    printf("EHCI: Initializing Controller at %x\n", g_EhciController.bar);
    printf("EHCI: Version %x, Ports: %d, Op Regs at: %x\n", 
           g_EhciController.hci_version, g_EhciController.n_ports, g_EhciController.op_reg_base);

    // 2. Stop the controller if it's running
    uint32_t usbcmd = mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBCMD);
    if (usbcmd & EHCI_CMD_RUN) {
        mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBCMD, usbcmd & ~EHCI_CMD_RUN);
    }

    // Wait for the Halt bit
    int timeout = 100;
    while (!(mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBSTS) & EHCI_STS_HALTED) && --timeout) {
        sleep_ms(1);
    }

    // 3. Reset the controller
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBCMD, EHCI_CMD_HCRESET);
    timeout = 100;
    while ((mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBCMD) & EHCI_CMD_HCRESET) && --timeout) {
        sleep_ms(1);
    }

    if (timeout == 0) {
        printf("EHCI: Controller reset failed!\n");
        return;
    }
    
    printf("EHCI: Reset successful.\n");

    // 3.5. Power on and Reset Ports
    // This is required to transition ports from 'Connected' to 'Enabled' (High Speed).
    for (uint32_t i = 0; i < g_EhciController.n_ports; i++) {
        uint32_t port_addr = g_EhciController.op_reg_base + EHCI_OP_PORTSC + (i * 4);
        uint32_t portsc = mmio_read32(port_addr);

        // Power on the port (bit 12)
        mmio_write32(port_addr, portsc | 0x1000);
        sleep_ms(20);

        portsc = mmio_read32(port_addr);
        if (portsc & 0x01) { // Device connected?
            // Initiate Port Reset (bit 8)
            mmio_write32(port_addr, portsc | 0x0100);
            sleep_ms(60);
            mmio_write32(port_addr, mmio_read32(port_addr) & ~0x0100);
            sleep_ms(20);

            // Wait for port to enable (bit 2)
            int reset_timeout = 20;
            while (--reset_timeout && !(mmio_read32(port_addr) & 0x04)) {
                sleep_ms(10);
            }

            // If port didn't enable, it's likely a Low/Full speed device.
            // Hand it off to the Companion Controller (bit 13: Port Owner)
            if (!(mmio_read32(port_addr) & 0x04)) {
                printf("EHCI: Port %d is not High-Speed. Handing off.\n", i);
                mmio_write32(port_addr, mmio_read32(port_addr) | 0x2000);
            }
        }
    }

    // 4. Set the CONFIGFLAG to route all ports to the EHCI controller
    // On systems with companion controllers (UHCI/OHCI), this is mandatory
    // to take control of the USB 2.0 ports.
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_CONFIGFLAG, 1);

    // 5. Basic setup: Clear interrupts and status
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBINTR, 0);
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBSTS, 0x3F); // Clear all status bits

    // 6. Initialize the Asynchronous Schedule circular list
    ehci_init_async_list();

    // Note: To actually read files, we would now need to:
    // a) Set up Periodic and Async lists (Memory structures)
    // b) Enumerate connected devices on the ports
    // c) Implement Bulk-Only Transport for Mass Storage
    
    // Start the controller
    usbcmd = mmio_read32(g_EhciController.op_reg_base + EHCI_OP_USBCMD);
    // Enable the controller (RUN) and the Asynchronous Schedule (ASYNC_EN)
    mmio_write32(g_EhciController.op_reg_base + EHCI_OP_USBCMD, usbcmd | EHCI_CMD_RUN | EHCI_CMD_ASYNC_EN);
    printf("EHCI: Controller started.\n");
}