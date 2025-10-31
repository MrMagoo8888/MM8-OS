#include "afk.h"
#include "stdio.h"
#include "string.h"


#include "commands/command.h"

#include "arch/i686/keyboard.h"
#include "mm8Splash.h"

void afk()
{   
    mm8Splash();

    printf("\n\n\nHello I am AFK for the moment\n\n\n\n\n");
    printf("Press any key to continue...\n");
    getch(); // Wait for any keypress
    printf("Welcome back!\n");
    return;
}