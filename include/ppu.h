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
                                         //
#define DOTS_PER_SCANLINE 456
#define VBLANK_START 144
#define VBLANK_END 154
#define DRAW_START 80
                                         //
enum {
    HBLANK = 0,
    VBLANK,
    OAM_SCAN,
    DRAW,
};

typedef struct object{
    uint8_t Y;
    uint8_t X;
    uint8_t tile_index;
    uint8_t sprite_flag;
} obj_t;

enum tile_type {
    OBJ = 0,
    BG,
    WIN,
};

/* 
 * The PPU struct is responsible for maintaining the state of the screen;
 * background: 32x32 tiles wide
 * window: the 2nd layer that can cover the background
 * memory_permissions a pointer into main_bus used to disable OAM and VRAM
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
    //I will try to go directly from memory for now
    //pixel_tile background[0x20][0x20];
    //pixel_tile window[0x20][0x20];
    byte* mem_perm_ptr;
    IRQ vblank_int;
    IRQ stat_int;
    struct ppu_context{
        uint8_t cur_x_cord;
        uint8_t cur_y_cord;
        address bg_idx_addr;
    } ctx;
    uint8_t WX;
    uint8_t WY;
    uint8_t SCX;
    uint8_t SCY;
    uint8_t LY;
    uint8_t LYC;
    obj_t obj_buf[10];
    uint8_t ob_idx;
    union {
        uint8_t data;
        struct {
            uint8_t disp_enabled: 1;
            uint8_t window_tile_map_select: 1;
            uint8_t window_disp_enabled: 1;
            uint8_t tile_data_select: 1;
            uint8_t bg_tile_map_select: 1;
            uint8_t sprite_size: 1;
            uint8_t sprite_enable: 1;
            uint8_t bg_window_enable: 1;
        } flags;
    } LCDC;
    union{
        uint8_t data;
        struct {
            uint8_t unused: 1;
            uint8_t LYC_stat_int: 1;
            uint8_t mode_2_int: 1;
            uint8_t mode_1_int: 1;
            uint8_t mode_0_int: 1;
            uint8_t coincidence_flag: 1; //set if LYC == LY
            uint8_t PPU_mode: 2;
        } flags;
    } STAT;
    uint16_t dot_counter; //to keep track of mode and adjust for pentalites
    int LCD_fifo_write; //I want to use a pipe to emulate the fifo pipe to the LCD
    pid_t lcd_pid;
} PPU_t;

PPU_t* init_ppu(byte* perm_ptr);
void ppu_cycle();
int cleanup_ppu();
byte read_LCDC();
void write_LCDC(byte data);
byte read_STAT();
void write_STAT(byte data);
byte read_SCX();
void write_SCX(byte data);
byte read_SCY();
void write_SCY(byte data);
byte read_WX();
void write_WX(byte data);
byte read_WY();
void write_WY(byte data);
byte read_LY();
byte read_LYC();
void write_LYC(byte data);

#endif
