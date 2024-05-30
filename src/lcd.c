#include "lcd.h"
#include <poll.h>
#include <raylib.h>
#include <stdint.h>
#include <string.h>

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

void color_correct(uint8_t *pix, uint8_t pallette){
    *pix = pallette >> (*pix * 2) & 3;
}

//bg_to_obj is only necessary for gameboy color
void merge(uint8_t x, uint8_t y, uint8_t obj_pix, bool priority){
    uint8_t bg_pix, merged;

    merged = 0;

    if(x < 8 || x > 168)
        return;

    bg_pix = lcd.screen[y][x-8];

    lcd.screen[y][x-8] = merged;
    if(priority){
        if(bg_pix > 0){
            merged = bg_pix;
        } else {
            merged = obj_pix;
        }
    } else {
        if(obj_pix > 0){
            merged = obj_pix;
        } else {
            merged = bg_pix;
        }
    }

    lcd.screen[y][x-8] = merged;
}


void parse_line(scanline *line, uint8_t y_idx){
    uint8_t x_idx, obj_pix, bg_pix, i, j;
    uint16_t reverse_row;
    sprite_t sprite;

    //color correct and store background data;
    x_idx = 0;
    for(i = 1; i < 21; i++){
        for(j = 0; j < 8; j++){
            bg_pix = (line->bg_data[i] >> j * 2) & 0x3;
            color_correct(&bg_pix, line->BGP);
            //printf("bg_pix corrected color: %d\n",bg_pix);
            lcd.screen[y_idx][x_idx] = bg_pix;
            x_idx++;
        }
    }

    for(i = 0; i < line->num_spt; i++){
        memcpy(&sprite, &line->spt_data[i], sizeof(sprite_t));
        if(sprite.flags.X_flip){
            reverse_row = 0;
            for(j = 0; j < 8; j++){
                reverse_row |= sprite.sprite_row >> (14 - (j*2)) & 3;
            }
            sprite.sprite_row = reverse_row;
        }
        for(j = 0; j < 8; j++){
            obj_pix = (sprite.sprite_row >> j * 2) & 0x3;
            color_correct(&obj_pix, sprite.pallette);
            merge(sprite.X + j, y_idx, obj_pix, sprite.flags.priority);
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
