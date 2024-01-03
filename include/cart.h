#ifndef CART_H
#define CART_H
#include "common.h"
#include <stdint.h>

//sourced from https://gbdev.io/pandocs/The_Cartridge_Header.html
typedef struct cart_struct{
    uint32_t entry;
    uint8_t logo[0x30];
    char title[0x10];
    uint32_t manufacturer_code;
    uint8_t CGB_flag;
    uint16_t new_licensee_code;
    uint8_t SBG_flag;
    uint8_t cart_type; //necessary for mapper
    uint8_t ROM_size; 
    uint8_t RAM_size; 
    uint8_t dest_code;
    uint8_t old_licensee_code;
    uint8_t mask_rom_version_numer;
    uint8_t header_checksum;
    uint16_t global_checksum;
} cart_t;

//read the header and s
void load_cart(cart_t* cart, char* filename);

#endif
