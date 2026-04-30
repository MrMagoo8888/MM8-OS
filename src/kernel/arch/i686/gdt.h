#pragma once

#define i686_GDT_CODE_SEGMENT 0x08
#define i686_GDT_DATA_SEGMENT 0x10
#define i686_GDT_USER_CODE_SEGMENT (0x18 | 3)
#define i686_GDT_USER_DATA_SEGMENT (0x20 | 3)
#define i686_GDT_TSS_SEGMENT 0x28

void i686_GDT_Initialize();
void i686_EnterUserMode(void* entryPoint, void* userStack);