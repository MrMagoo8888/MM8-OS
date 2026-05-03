#include "ehci.h"
#include "stdio.h"
#include "stddef.h"
#include "time.h"

int usb_init_device_msc(ehci_controller_t* hc) {
    usb_setup_packet_t setup;
    uint8_t max_lun = 0;
    
    printf("USB: Starting from-scratch enumeration...\n");

    // Stage 1: Set Address (Move from Default to Addressed state)
    setup.request_type = 0x00; // Device
    setup.request = USB_REQ_SET_ADDRESS;
    setup.value = 1;
    setup.index = 0;
    setup.length = 0;
    if (ehci_control_transfer(hc, 0, &setup, NULL) != 0) return -1;
    sleep_ms(10);

    // Stage 2: Set Configuration (Move to Configured state)
    setup.request_type = 0x00;
    setup.request = USB_REQ_SET_CONFIGURATION;
    setup.value = 1;
    setup.index = 0;
    setup.length = 0;
    if (ehci_control_transfer(hc, 1, &setup, NULL) != 0) return -1;
    sleep_ms(10);

    // Stage 3: Get Max LUN (Mass Storage Class Specific Request)
    // Many drives stall if this isn't performed.
    setup.request_type = 0xA1; // Class, Interface, Recipient
    setup.request = 0xFE;      // GET_MAX_LUN
    setup.value = 0;
    setup.index = 0;
    setup.length = 1;
    ehci_control_transfer(hc, 1, &setup, &max_lun);

    printf("USB: Device configured. Max LUN: %d\n", max_lun);
    return 0;
}