#include "io.h"

#define UNUSED_PORT         0x80

void x86_64_iowait()
{
    x86_64_outb(UNUSED_PORT, 0);
}