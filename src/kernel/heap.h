#pragma once

#include <stdint.h>
#include <stddef.h>

void heap_initialize();
void heap_get_stats(size_t* total, size_t* used, size_t* free);