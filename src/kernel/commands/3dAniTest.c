#include "stdio.h"
#include "3dAniTest.h"
#include "string.h"
#include "graphics.h" //raphics library
#include "arch/i686/keyboard.h"

void squareTest() {
    clrscr();
    printf("3D Animation Test - Square\n");

    int size = 5;
    int posX = (SCREEN_WIDTH - size) / 2;
    int posY = (SCREEN_HEIGHT - size) / 2;
    
    draw_rect(posX, posY, size, size, '#', Color_LightGreen);

    setcursor(0, SCREEN_HEIGHT - 1);
}

void circleTest() {
    // clrscr() is removed so we can draw over the square
    printf("3D Animation Test - Circle\n");

    int radius = 5;
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    draw_circle(centerX, centerY, radius, 'O', Color_LightBlue);

    setcursor(0, SCREEN_HEIGHT - 1);
}

void ani3d_test() {
    squareTest();

    getch();
    clrscr();

    circleTest();

    getch();
    clrscr();

    draw_circle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 8, '*', Color_Yellow);
}
