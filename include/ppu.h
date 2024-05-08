#ifndef PPU_H
#define PPU_H
#include "common.h"
#include "main_bus.h"
#include "lcd.h"
#include <stdint.h>

#define HIEGHT 160
#define WIDTH 144

//relavant VRAM addresses
#define BACKGROUND1 0x9800
#define BACKGROUND2 0x9C00

////to help keep information straight
//typedef uint8_t pixel_row[8];
//typedef pixel_row pixel_tile[8];
//typedef pixel_tile sprite;
//typedef pixel_tile tall_sprite[2];

//each row is determend by combining the first and second bytes
#define PIXEL_ROW_SIZE 2
#define PIXEL_TILE_SIZE 8*PIXEL_ROW_SIZE //8x8 x 2bits per pixel / 8 total space = 16bytes

typedef struct object{
    uint8_t Y;
    uint8_t X;
    uint8_t tile_index;
    uint8_t sprite_flag;
} obj_t;

/* 
 * The PPU struct is responsible for maintaining the state of the screen;
 * background: 32x32 tiles wide
 * window: the 2nd layer that can cover the background
 * WX: X-cord of the window
 * WY: Y-cord of the window
 * SCX: X-cord of the viewport
 * SCY: Y-cord of the viewport
 * LY: current position of the scanner
 * obj_buf: contains the objects currently being drawn by the scanline, max of 10
 * ob_idx: contains the index of the next object
 * LCDC: Control register for the LCD, for information on bits see https://hacktix.github.io/GBEDG/ppu/
 * STAT: contains the status of the PPU
 * lcd_pid: pid of process running the LCD
 */
typedef struct PPU_struct{
    pixel_tile background[0x20][0x20];
    pixel_tile window[0x20][0x20];
    uint8_t WX;
    uint8_t WY;
    uint8_t SCX;
    uint8_t SCY;
    uint8_t LY;
    obj_t obj_buf[10];
    uint8_t ob_idx;
    struct {
        uint8_t disp_enabled: 1;
        uint8_t window_tile_map_select: 1;
        uint8_t window_disp_enabled: 1;
        uint8_t tile_data_select: 1;
        uint8_t bg_tile_map_select: 1;
        uint8_t sprite_size: 1;
        uint8_t sprite_enable: 1;
        uint8_t bg_window_enable: 1;
    } LCDC;
    struct {
        uint8_t unused: 1;
        uint8_t LYC_stat_int: 1;
        uint8_t mode_2_int: 1;
        uint8_t mode_1_int: 1;
        uint8_t mode_0_int: 1;
        uint8_t coincidence_flag: 1; //set if LYC == LY
        uint8_t PPU_mode: 2;
    } STAT;
    uint8_t mode;
    uint32_t current_cycle;
    int LCD_fifo_write; //I want to use a pipe to emulate the fifo pipe to the LCD
    pid_t lcd_pid;
} PPU_t;

PPU_t* init_ppu();
int cleanup_ppu();
#endif
