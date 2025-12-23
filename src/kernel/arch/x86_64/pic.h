#pragma once

#include <stdint.h>

void x86_64_PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2);
void x86_64_PIC_SendEndOfInterrupt(int irq);
void x86_64_PIC_Disable();
void x86_64_PIC_Mask(int irq);
void x86_64_PIC_Unmask(int irq);
uint16_t x86_64_PIC_ReadIrqRequestRegister();
uint16_t x86_64_PIC_ReadInServiceRegister();