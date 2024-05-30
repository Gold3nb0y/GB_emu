#ifndef LCD_H
#define LCD_H
#include "common.h"
#include "signal.h"
#include <stdint.h>

/*
 * my idea for this LCD class is to fork into a process that will consume writes from the
 * and display them to the window, by isolating the LCD I remove so of the timing restrictions
 * there might be to draw to the screen
 */
#define SCRN_WIDTH 160
#define SCRN_HEIGHT 144
#define BG_WIDTH 256
#define BG_HEIGHT 256
#define SCALE 3 //scale up the image to be a bit larger

typedef struct spt_data{
    uint16_t sprite_row;
    uint8_t X;
    uint8_t pallette;
    union{
        uint8_t sprite_flag;
        struct {
            uint8_t priority: 1;
            uint8_t Y_flip: 1;
            uint8_t X_flip: 1;
            uint8_t DMG_pallette: 1;
            uint8_t bank: 1;
            uint8_t cgb_pallette: 3;
        } flags;
    };
} sprite_t;

typedef struct {
    uint16_t bg_pixels[160];
    sprite_t spt_data[10];
    uint8_t num_spt;
    uint8_t BGP;
    bool bg_to_obj;
} scanline;


/*
 * lcd_fifo_read: read side of the pipe, allows the lcd to work as a consumer
 * ppu: pointer to the ppu so I can change the STAT register if needbe
 */
typedef struct lcd_struct {
    int lcd_fifo_read;
    uint8_t screen[SCRN_HEIGHT][SCRN_WIDTH]; //store all information that is read from the ppu
} LCD_t;



void init_lcd(int read_fd);
void lcd_loop();
#endif
