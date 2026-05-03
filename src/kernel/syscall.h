#pragma once
#include <stdint.h>

// Syscall Numbers
#define SYS_EXIT  0
#define SYS_PUTS  1
#define SYS_GETS  2
#define SYS_MALLOC 3
#define SYS_FREE   4

void syscall_initialize();