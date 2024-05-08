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
    lcd.lcd_fifo_read = read_fd;    
    return;
}

void recv_fifo(byte *data){
    read(lcd.lcd_fifo_read, data, 1);
    return;
}

void write_data(byte data){
    lcd.screen[lcd.row][lcd.col] = data;

    //check bounds
    lcd.col++;
    if(lcd.col >= SCRN_WIDTH * 8){
        lcd.col = 0;
        lcd.row++;
        if(lcd.row >= SCRN_HEIGHT *8)
            lcd.row = 0;
    }
}

void render_screen(){
    for(uint64_t i = 0; i < SCRN_WIDTH; i++){
        for(uint64_t j = 0; j < SCRN_HEIGHT; j++){
            //TODO fetch the color from the pixel stored at that location
            DrawRectangle(i * SCALE, j * SCALE, SCALE, SCALE, WHITE);
        }
    }
}

void lcd_loop(){
    struct pollfd events = {lcd.lcd_fifo_read, POLLIN, 0};
    byte data;
    while(!WindowShouldClose()){
        if(poll(&events, 1, 0)){
            recv_fifo(&data);
            write_data(data);
            //do something to store the data
        } else {
            BeginDrawing();
            render_screen();
            EndDrawing();
        }
    }
    cleanup_lcd(0);
}
