#include "ppu.h"
#include <stdint.h>

static PPU_t ppu;

void send_pixel(byte data){
    write(ppu.LCD_fifo_write, &data, 1);
    return;
}

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

void parse_pixel_tile_from_memory(address addr, pixel_tile *tile){
    uint16_t cur_row;
    pixel_row *row = (pixel_row*)tile;
    for(uint32_t i = 0; i < 0x8; i++){
        cur_row = read_bus_addr(addr);
        addr += 2;
        parse_pixel_row(cur_row, &row[i]);
    }
    //do something with the tile containing the pixel information
    return;
}

address get_tile_address(uint8_t tile_index, bool is_sprite){
    address ret;
    //depending on the value of the LCDC register the lookup changes
    LOG(ERROR, "lookup not fully implemented");
    exit(-1);
    if(ppu.LCDC.window_tile_map_select && !is_sprite){
        ret = VRAM_START + 0x1000 + ((int8_t)tile_index * 0x10); 
    } else {
        ret = VRAM_START + (tile_index * 0x10); 
    }
    return ret;
}

//this parses and loads the information in memory into the struct I have availible
//void load_background(address addr, pixel_tile *area){
//    for(uint32_t i = 0; i < 0x40; i++){
//        parse_pixel_tile_from_memory(addr+i*0x10, &area[i]);
//    }
//}

void read_obj(uint8_t idx, obj_t *obj){
    obj->Y = read_bus(OAM_START + idx * sizeof(obj_t));
    obj->X = read_bus(OAM_START + idx * sizeof(obj_t)+1);
    obj->tile_index = read_bus(OAM_START + idx * sizeof(obj_t)+2);
    obj->sprite_flag = read_bus(OAM_START + idx * sizeof(obj_t)+3);
    return;
}

struct sprite_stack {
    obj_t to_display[10]; //up to 10 sprites can be displayed for any 1 scanline
    uint8_t count;
    uint8_t current;
};

struct sprite_stack sprites = {0};

//takes 80 cycles
void scan_OAM(uint16_t scanline){
    uint8_t count = 0;
    obj_t tmp_obj = {0};
    uint8_t sprite_size = 0;
    memset(&sprites, 0, sizeof(struct sprite_stack)); //reset the displaybuffer
    for(uint8_t i = 0; i < 40; i++){
        read_obj(i, &tmp_obj);
        if(tmp_obj.X > 0 && scanline+16 >= tmp_obj.Y && count < 10){
            sprite_size = ppu.LCDC.sprite_size ? 16 : 8;
            if(scanline+16 < tmp_obj.Y+sprite_size){
                memcpy(&sprites.to_display[count], &tmp_obj, sizeof(obj_t));
                count++;
            }
        }
    }
}

uint8_t calc_bg_idx(uint8_t x_pos){
    address addr;
    uint8_t tile_idx;

    if(ppu.LCDC.window_disp_enabled){
        //do something
        addr = 0;
    } else {
        //fetch bg tile number
        addr = ppu.LCDC.bg_tile_map_select ? BACKGROUND2 : BACKGROUND1;
        addr += (x_pos + (ppu.SCX / 8)) & 0x1f;
        addr += 32 * (((ppu.LY + ppu.SCY) & 0xFF) / 8);
    }
    tile_idx = read_bus(addr);
    return tile_idx;
}

//send all of the bytes in order
void scanline(uint16_t scanline){
    byte to_send;
    uint16_t fetched_bg = 0;
    uint32_t fetched_sprite = 0;
    uint8_t x_pos = 0;
    address read_from = 0;
    bool sprite_offset;

    scan_OAM(scanline); //populate the sprite fifo
                        
    while(x_pos <= 0x20){
        // fetch background
        read_from = get_tile_address(calc_bg_idx(x_pos), false);
        fetched_bg = read_bus_addr(read_from + ((ppu.LY + ppu.SCY) % 8)); 

        // fetch sprite 
        for(uint8_t i = 0; i < 10; i++){
            if(sprites.to_display[i].X <= (x_pos+1)*8){
                read_from = get_tile_address(sprites.to_display[i].tile_index, true);
                fetched_sprite = read_bus_addr(read_from + ((ppu.LY + ppu.SCY) % 8)); 
                sprite_offset = sprites.to_display[i].X % 8;
                fetched_sprite |= fetched_sprite << sprite_offset*2; //get the sprite into the correct position
            }
        }

        //send pixels to LCD;
        x_pos++; 
    }
}

//this might change later if the piping doesn't work out, but for now I think this looks good
PPU_t* init_ppu(){
    int fifo[2];
    pid_t pid;
    if(pipe(fifo) == -1){
        LOG(ERROR, "Failed to create pipe for ppu and lcd");
        exit(1);
    }
    pid = fork();
    if(!pid){
        close(fifo[1]);
        init_lcd(fifo[0]);
        lcd_loop();
    } else {
        close(fifo[0]);
        ppu.LCD_fifo_write = fifo[1];
        ppu.lcd_pid = pid;
        ppu.STAT.unused = 1; //must be set to one according to documentation
    }
    return &ppu;
}

uint64_t ppu_cycle(uint64_t cycles, int64_t ticks){
    for(;ticks < 0; ticks--){
        
        cycles++;
    }
    return cycles;
}

int cleanup_ppu(){
    close(ppu.LCD_fifo_write);
    return 0;
}
