#include "stdint.h"
#include "stdio.h"
#include "memory.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    clrscr();

    printf("Hello world from kernel!!!\n");

    apptest();

    

end:
    for (;;);
}

void apptest()
{
    printf("Hello from app test!\n");

    printf("Testing printf: %d, %x, %c, %s\n", 1234, 0xABCD, 'A', "Hello");

    printf("Press One to open App 1\n");
    printf("Press Two to open App 2\n");
    printf("Press Three to open App 3\n");

}