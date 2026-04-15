#include "noCrash.h"
#include "stdio.h"
#include "graphics.h"
#include <commands/color.h>

void init_tests() {
    pixelLoop();
}

void pixelLoop() {

    while (1) 
    {
        // Makes looping loding-like circle out of a few pixels
        draw_pixel(1900, 1060, Color_DarkGray); //Set Top Left
        draw_pixel(1900, 1060, BACKGROUND_COLOR); //Clear Top Right
        draw_pixel(1900, 1050, Color_DarkGray); //Set Bottom Left

        draw_pixel(1900, 1060, BACKGROUND_COLOR); //Clear Top Left
        draw_pixel(1900, 1050, Color_DarkGray); //Set Bottom Right
        draw_pixel(1900, 1050, BACKGROUND_COLOR); //Clear Bottom Left

        draw_pixel(1900, 1060, Color_DarkGray); //Set Top Right
        draw_pixel(1900, 1050, BACKGROUND_COLOR); //Clear Bottom Right
        
    }
}