#ifndef PPU_H
#define PPU_H
#include "common.h"
#include "main_bus.h"
#include <stdint.h>

#define HIEGHT 160
#define WIDTH 144

//relavant VRAM addresses
#define BACKGROUND1 0x9800
#define BACKGROUND2 0x9C00

//to help keep information straight
typedef uint8_t pixel_row[8];
typedef pixel_row pixel_tile[8];
typedef pixel_tile sprite;
typedef pixel_tile tall_sprite[2];

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
}PPU_t;
#endif
