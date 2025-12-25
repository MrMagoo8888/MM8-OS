#pragma once

void gdt_initialize();

#define X86_64_GDT_CODE_SEGMENT 0x08
#define X86_64_GDT_DATA_SEGMENT 0x10