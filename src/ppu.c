#include "ppu.h"
#include <stdint.h>

void parse_pixel_row(uint16_t row, pixel_row *values){
    uint8_t a,b;
    uint8_t mask;
    a = row >> 8;
    b = row & 0xFF;

    //shift values into the correct position
    for(uint32_t i = 0; i < 0x8; i++){
        mask = 1 << i;
        (*values)[i] = (a & mask) >> i;
        (*values)[i] |= ((b & mask) >> i) << 1;
    }
    return;
}

void parse_pixel_tile(address addr, pixel_tile *tile){
    uint16_t cur_row;
    for(uint32_t i = 0; i < 0x8; i++){
        cur_row = read_bus_addr(addr);
        addr += 2;
        parse_pixel_row(cur_row, (pixel_row *)(tile + (i * sizeof(pixel_row)))); //recast is the only way I know how to do this
    }
    //do something with the tile containing the pixel information
    return;
}

address lookup_tile(uint8_t tile_index){
    address ret;
    //depending on the value of the LCDC register the lookup changes
    LOG(ERROR, "lookup not fully implemented");
    exit(-1);
    if(true){
        ret = VRAM_START + (tile_index * 0x10); 
    } else {
        ret = VRAM_START + 0x1000 + ((int8_t)tile_index * 0x10); 
    }
    return ret;
}

//this parses and loads the information in memory into the struct I have availible
void load_background(address addr, pixel_tile *area){
    for(uint32_t i = 0; i < 0x40; i++){
        parse_pixel_tile(addr+i*0x10, &area[i]);
    }
}

void read_obj(address addr, obj_t *obj){
    obj->Y = read_bus(addr);
    obj->X = read_bus(addr+1);
    obj->tile_index = read_bus(addr+2);
    obj->sprite_flag = read_bus(addr+3);
    return;
}
