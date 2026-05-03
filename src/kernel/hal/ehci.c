#include "ehci.h"
#include "stdio.h"
#include "memory.h"
#include "time.h"
#include "globals.h"

void ehci_init(pci_device_t* dev) {
    // 1. Get MMIO Base from BAR0
    uint32_t bar0 = pci_read_config(dev->bus, dev->device, dev->function, 0x10);
    if (bar0 & 1) {
        printf("EHCI: Error - BAR0 is I/O, expected MMIO\n");
        return;
    }

    ehci_controller_t* hc = (ehci_controller_t*)malloc(sizeof(ehci_controller_t));
    if (!hc) return;

    hc->cap_base = bar0 & 0xFFFFFFF0;
    hc->pci_dev = *dev;

    // Read CAPLENGTH to find where Operational Registers start
    uint8_t cap_length = *(volatile uint8_t*)(hc->cap_base + EHCI_CAP_CAPLENGTH);
    hc->op_base = hc->cap_base + cap_length;

    // 2. Enable PCI Bus Mastering
    uint32_t command = pci_read_config(dev->bus, dev->device, dev->function, 0x04);
    pci_write_config(dev->bus, dev->device, dev->function, 0x04, command | 0x06);

    // 3. Reset Controller
    // Ensure controller is stopped before resetting
    uint32_t usbcmd = ehci_read_op(hc, EHCI_OP_USBCMD);
    usbcmd &= ~EHCI_CMD_RUN;
    ehci_write_op(hc, EHCI_OP_USBCMD, usbcmd);
    
    // Wait for HCHalted bit
    uint32_t timeout = 1000000;
    while (!(ehci_read_op(hc, EHCI_OP_USBSTS) & EHCI_STS_HALTED) && --timeout);
    if (timeout == 0) {
        printf("EHCI: Warning - Controller did not halt.\n");
    }

    // Trigger Reset
    ehci_write_op(hc, EHCI_OP_USBCMD, EHCI_CMD_HCRESET);
    timeout = 1000000;
    while ((ehci_read_op(hc, EHCI_OP_USBCMD) & EHCI_CMD_HCRESET) && --timeout); // Wait for reset to complete
    if (timeout == 0) {
        printf("EHCI: Error - Controller reset timed out.\n");
        return;
    }

    // 4. Configure Port Routing
    // Setting CONFIGFLAG to 1 routes all ports to the EHCI controller
    ehci_write_op(hc, EHCI_OP_CONFIGFLAG, 1);

    // 5. Initialize Async List (Circular list with one dummy QH)
    hc->async_qh = (ehci_qh_t*)malloc_aligned(sizeof(ehci_qh_t), 32);
    memset(hc->async_qh, 0, sizeof(ehci_qh_t));
    
    // Dummy QH points to itself and has the 'H' bit set
    hc->async_qh->horizontal_link = ((uint32_t)hc->async_qh) | 0x02; // Type = QH
    hc->async_qh->endpoint_char = QH_H | QH_SPEED_HIGH; 
    hc->async_qh->next_qtd = 1; // Terminate
    hc->async_qh->alt_next_qtd = 1; // Terminate

    ehci_write_op(hc, EHCI_OP_ASYNCLISTADDR, (uint32_t)hc->async_qh);
    
    // Enable Async Schedule
    // We must set EHCI_CMD_RUN (Run/Stop bit) so the controller starts processing the list
    ehci_write_op(hc, EHCI_OP_USBCMD, ehci_read_op(hc, EHCI_OP_USBCMD) | EHCI_CMD_RUN | EHCI_CMD_ASYNCE);
    timeout = 1000000;
    while (!(ehci_read_op(hc, EHCI_OP_USBSTS) & EHCI_STS_ASYNCS) && --timeout);

    // 6. Get port count and power them up
    uint32_t hcs_params = *(volatile uint32_t*)(hc->cap_base + EHCI_CAP_HCSPARAMS);
    hc->num_ports = hcs_params & 0x0F;

    printf("EHCI: Controller initialized. Ports: %d\n", hc->num_ports);

    // 6. Basic Port Check
    for (int i = 0; i < hc->num_ports; i++) {
        uint32_t port_reg = EHCI_OP_PORTSC + (i * 4);
        uint32_t status = ehci_read_op(hc, port_reg);
        
        // Power on port if it supports it
        if (hcs_params & (1 << 4)) { 
            status |= (1 << 12); // Port Power bit
            ehci_write_op(hc, port_reg, status);
            sleep_ms(20);
        }

        // Perform Port Reset (Mandatory to move device to 'Default' state)
        status = ehci_read_op(hc, port_reg);
        if (status & 0x01) { // Device connected
            status |= (1 << 8); // Port Reset bit
            ehci_write_op(hc, port_reg, status);
            sleep_ms(50);
            status &= ~(1 << 8);
            ehci_write_op(hc, port_reg, status);
            sleep_ms(20);
        }

        status = ehci_read_op(hc, port_reg);
        if (status & 0x01) { // Current Connect Status
            printf("EHCI: Device detected on port %d\n", i);
            
            // Claim as system disk if none exists
            if (g_Disk.type == DISK_TYPE_ATA) {
                g_Disk.type = DISK_TYPE_USB;
                g_Disk.driver_data = hc;
            }
        }
    }

    // Note: To actually transfer data, we'd need to set up Periodic/Async Lists here.
}

int ehci_bulk_transfer(ehci_controller_t* hc, uint8_t addr, uint8_t ep, void* data, uint32_t len, int is_in) {
    // 1. Create a qTD for the data
    ehci_qtd_t* qtd = (ehci_qtd_t*)malloc_aligned(sizeof(ehci_qtd_t), 32);
    memset(qtd, 0, sizeof(ehci_qtd_t));
    
    qtd->next_qtd = 1;      // Terminate
    qtd->alt_next_qtd = 1;  // Terminate
    
    // Token: Total Bytes, IOC, PID, Status=Active
    uint32_t pid = is_in ? QTD_PID_IN : QTD_PID_OUT;
    qtd->token = (len << 16) | (3 << 12) | (pid << 8) | QTD_ACTIVE | QTD_IOC;
    
    // Set buffer pointers (Properly handling 4KB page boundaries)
    uint32_t cur_addr = (uint32_t)data;
    qtd->buffer[0] = cur_addr;
    for (int i = 1; i < 5; i++) {
        // Calculate the start of the next physical page
        uint32_t next_page = (cur_addr + 4096) & ~0xFFF;
        qtd->buffer[i] = next_page;
        cur_addr = next_page;
    }

    // 2. Create a temporary QH for this transfer
    ehci_qh_t* qh = (ehci_qh_t*)malloc_aligned(sizeof(ehci_qh_t), 32);
    memset(qh, 0, sizeof(ehci_qh_t));
    
    qh->endpoint_char = (512 << 16) | QH_DTC | QH_SPEED_HIGH | (ep << 8) | addr;
    qh->endpoint_caps = (1 << 30); // Mult = 1
    qh->current_qtd = 0;
    qh->next_qtd = (uint32_t)qtd;
    
    // 3. Insert into the Async List
    // Link new QH to the old horizontal link, then point dummy head to new QH
    uint32_t prev_link = hc->async_qh->horizontal_link;
    qh->horizontal_link = prev_link;
    hc->async_qh->horizontal_link = ((uint32_t)qh) | 0x02;
    
    // Memory barrier to ensure the controller sees the updated list
    __asm__ volatile("" ::: "memory");

    // 4. Wait for transfer to complete (Active bit cleared by hardware)
    uint32_t timeout = 1000000;
    while (((*(volatile uint32_t*)&qtd->token) & QTD_ACTIVE) && --timeout) {
        __asm__ volatile("pause");
    }

    // 5. Clean up and remove from list
    hc->async_qh->horizontal_link = prev_link;
    
    // Small delay to ensure the controller has finished with the QH 
    // before we free the memory.
    sleep_ms(2);
    
    int success = (timeout > 0 && !(qtd->token & 0x7C)); // Check error bits
    
    free(qh);
    free(qtd);
    
    return success ? 0 : -1;
}

int ehci_control_transfer(ehci_controller_t* hc, uint8_t addr, usb_setup_packet_t* setup, void* data) {
    // Control transfers have 3 stages: SETUP, DATA (optional), and STATUS.
    // 1. Create qTD for SETUP stage
    ehci_qtd_t* q_setup = (ehci_qtd_t*)malloc_aligned(sizeof(ehci_qtd_t), 32);
    memset(q_setup, 0, sizeof(ehci_qtd_t));
    q_setup->token = (sizeof(usb_setup_packet_t) << 16) | (3 << 12) | (QTD_PID_SETUP << 8) | QTD_ACTIVE;
    q_setup->buffer[0] = (uint32_t)setup;

    ehci_qtd_t* last_qtd = q_setup;

    // 2. Optional DATA stage
    ehci_qtd_t* q_data = NULL;
    if (setup->length > 0) {
        q_data = (ehci_qtd_t*)malloc_aligned(sizeof(ehci_qtd_t), 32);
        memset(q_data, 0, sizeof(ehci_qtd_t));
        uint32_t data_pid = (setup->request_type & 0x80) ? QTD_PID_IN : QTD_PID_OUT;
        q_data->token = (setup->length << 16) | (3 << 12) | (data_pid << 8) | QTD_ACTIVE;
        q_data->buffer[0] = (uint32_t)data;
        q_data->next_qtd = 1;
        
        last_qtd->next_qtd = (uint32_t)q_data;
        last_qtd = q_data;
    }

    // 3. STATUS stage
    ehci_qtd_t* q_status = (ehci_qtd_t*)malloc_aligned(sizeof(ehci_qtd_t), 32);
    memset(q_status, 0, sizeof(ehci_qtd_t));
    uint32_t status_pid = (setup->length == 0 || (setup->request_type & 0x80)) ? QTD_PID_OUT : QTD_PID_IN;
    q_status->token = (0 << 16) | (3 << 12) | (status_pid << 8) | QTD_ACTIVE | QTD_IOC;
    q_status->next_qtd = 1;

    last_qtd->next_qtd = (uint32_t)q_status;

    // 4. Create and Insert QH
    ehci_qh_t* qh = (ehci_qh_t*)malloc_aligned(sizeof(ehci_qh_t), 32);
    memset(qh, 0, sizeof(ehci_qh_t));
    qh->endpoint_char = (64 << 16) | QH_H | QH_DTC | QH_SPEED_HIGH | (0 << 8) | addr;
    qh->next_qtd = (uint32_t)q_setup;

    uint32_t prev_link = hc->async_qh->horizontal_link;
    qh->horizontal_link = prev_link;
    hc->async_qh->horizontal_link = ((uint32_t)qh) | 0x02;

    __asm__ volatile("" ::: "memory");

    uint32_t timeout = 1000000;
    while (((*(volatile uint32_t*)&q_status->token) & QTD_ACTIVE) && --timeout) {
        __asm__ volatile("pause");
    }

    hc->async_qh->horizontal_link = prev_link;
    sleep_ms(2);

    int success = (timeout > 0 && !(q_status->token & 0x7C));

    free(q_setup);
    if (q_data) free(q_data);
    free(q_status);
    free(qh);

    return success ? 0 : -1;
}