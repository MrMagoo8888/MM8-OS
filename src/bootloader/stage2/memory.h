#pragma once
#include "stdint.h"

void* memcpy(void* dst, const void* src, uint32_t num);
void* memset(void* ptr, int value, uint32_t num);
int memcmp(const void* ptr1, const void* ptr2, uint32_t num);
// may have unwanted and unintended consequences cause of a and b being evaluated multiple times; plz fix later me. THX!!!