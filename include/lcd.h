#ifndef LCD_H
#define LCD_H
#include "common.h"
#include "signal.h"

/*
 * my idea for this LCD class is to fork into a process that will consume writes from the
 * and display them to the window, by isolating the LCD I remove so of the timing restrictions
 * there might be to draw to the screen
 */

/*
 * lcd_fifo_read: read side of the pipe, allows the lcd to work as a consumer
 * ppu: pointer to the ppu so I can change the STAT register if needbe
 */
typedef struct lcd_struct {
    int lcd_fifo_read;
} LCD_t;


void init_lcd(int read_fd);
void lcd_loop();
#endif
