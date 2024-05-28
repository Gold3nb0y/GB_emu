#include "lcd.h"
#include <poll.h>
#include <raylib.h>
#include <stdint.h>

LCD_t lcd;

void cleanup_lcd(int sig){
    LOG(INFO, "Cleaning up the LCD handler");
    CloseWindow();
    close(lcd.lcd_fifo_read);
    exit(0);
}

//TODO create window for raylib, set a signal listener to kill the LCD process
void init_lcd(int read_fd){
    InitWindow(SCRN_WIDTH * SCALE, SCRN_HEIGHT * SCALE, "Game Boi");
    LOG(INFO, "LCD created");
    signal(SIGPIPE, cleanup_lcd); //set a cleanup listener
    memset(&lcd, 0, sizeof(LCD_t));
    lcd.lcd_fifo_read = read_fd;    
    return;
}

void recv_fifo(scanline *line){
    read(lcd.lcd_fifo_read, line, sizeof(scanline));
    return;
}

//bg_to_obj is only necessary for gameboy color
uint8_t merge(uint8_t bg_pix, uint8_t obj_pix, bool bg_to_obj){
    uint8_t merged;
    if(obj_pix != 0){
        merged = obj_pix;
    } else {
        merged = bg_pix;
    }
    return merged;
}

void parse_line(scanline *line, uint8_t y_idx){
    uint8_t x_idx, merged, obj_pix, bg_pix, i, j;

    x_idx = 0;
    for(i = 1; i < 21; i++){
        for(j = 0; j < 8; j++){
            bg_pix = (line->bg_data[i] >> j * 2) & 0x3;
            obj_pix = (line->sprite_data[i] >> j * 2) & 0x3;
            merged = merge(bg_pix, obj_pix, line->bg_to_obj);
            lcd.screen[y_idx][x_idx] = merged;
            x_idx++;
        }
    }
}

void render_screen(){
    uint8_t pix;
    for(uint64_t i = 0; i < SCRN_HEIGHT; i++){
        for(uint64_t j = 0; j < SCRN_WIDTH; j++){
            pix = lcd.screen[i][j];
            //TODO fetch the color from the pixel stored at that location
            switch(pix){
                case 0:
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, WHITE);
                    break;
                case 1:
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, LIGHTGRAY);
                    break;
                case 2:
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, DARKGRAY);
                    break;
                case 3:
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, BLACK);
                    break;
                default:
                    printf("error in lcd pix 0x%x\n", pix);
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, WHITE);
                    break;
            }
        }
    }
}

void lcd_loop(){
    struct pollfd events = {lcd.lcd_fifo_read, POLLIN, 0};
    uint8_t y = 0;
    scanline line;
    while(!WindowShouldClose()){
        if(poll(&events, 1, 0)){
            recv_fifo(&line);
            parse_line(&line, y);
            y++;
            if(y == 144) y = 0;
            //do something to store the data
        } 
        BeginDrawing();
        render_screen();
        EndDrawing();
    }
    cleanup_lcd(0);
}
