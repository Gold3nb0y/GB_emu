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
    *pix = (pallette >> (*pix * 2)) & 3;
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


void parse_line(scanline *line){
    uint8_t x_idx, obj_pix, bg_pix, i, j;
    uint16_t reverse_row;
    sprite_t sprite;

    //color correct and store background data;
    for(i = 0; i < 160; i++){
        bg_pix = line->bg_pixels[i];
        color_correct(&bg_pix, line->BGP);
        //printf("bg_pix corrected color: %d\n",bg_pix);
        lcd.screen[line->Y][i] = bg_pix;
    }

    for(i = 0; i < line->num_spt; i++){
        memcpy(&sprite, &line->spt_data[i], sizeof(sprite_t));
        for(j = 0; j < 8; j++){
            obj_pix = sprite.flags.X_flip ? sprite.pixels[7-j] : sprite.pixels[j];
            color_correct(&obj_pix, sprite.pallette);
            merge(sprite.X + j, line->Y, obj_pix, sprite.flags.priority);
        }
    }
}

void draw_grid(){
    uint64_t i;

    for(i = 0; i < 20; i++)
        DrawLine((i * 8) * SCALE, 0, (i * 8) * SCALE, 144 * SCALE, RED);

    for(i = 0; i < 18; i++)
        DrawLine(0, (i * 8) * SCALE, 160 * SCALE, (i * 8) * SCALE, RED);
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
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, BLUE);
                    break;
                case 2:
                    DrawRectangle(j * SCALE, i * SCALE, SCALE, SCALE, PURPLE);
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
    //draw_grid();
}

void lcd_loop(){
    struct pollfd events = {lcd.lcd_fifo_read, POLLIN, 0};
    scanline line;
    while(!WindowShouldClose()){
        if(poll(&events, 1, 0)){
            recv_fifo(&line);
            parse_line(&line);
        } 
        BeginDrawing();
        render_screen();
        EndDrawing();
    }
    cleanup_lcd(0);
}
