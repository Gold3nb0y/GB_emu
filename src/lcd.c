#include "lcd.h"

LCD_t lcd;

void cleanup_lcd(int sig){
    LOG(INFO, "Cleaning up the LCD handler");
    close(lcd.lcd_fifo_read);
    exit(0);
}

//TODO create window for raylib, set a signal listener to kill the LCD process
void init_lcd(int read_fd){
    LOG(INFO, "LCD created");
    signal(SIGPIPE, cleanup_lcd); //set a cleanup listener
    lcd.lcd_fifo_read = read_fd;    
    return;
}

void lcd_loop(){
    for(;;){
        ;
    }
}
