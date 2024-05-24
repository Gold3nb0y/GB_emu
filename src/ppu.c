#include "ppu.h"
#include <stdint.h>

static PPU_t ppu;

//The following functions are used for IO_registers
byte read_LCDC(){
    return ppu.LCDC.data;
}

void write_LCDC(byte data){
    ppu.LCDC.data = data;
}

byte read_STAT(){
    return ppu.STAT.data;
}

void write_STAT(byte data){
    ppu.STAT.data |= data & 0xf8;
}

byte read_SCX(){
    return ppu.SCX;
}

void write_SCX(byte data){
    ppu.SCX = data;
}

byte read_SCY(){
    return ppu.SCY;
}

void write_SCY(byte data){
    ppu.SCY = data;
}

byte read_WX(){
    return ppu.WX;
}

void write_WX(byte data){
    ppu.WX = data;
}

byte read_WY(){
    return ppu.WY;
}

void write_WY(byte data){
    ppu.WY = data;
}

byte read_LY(){
    return ppu.LY;
}

byte read_LYC(){
    return ppu.LYC;
}

void write_LYC(byte data){
    ppu.LYC = data;
}

//start of functions used for ppu implementations
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
    if(ppu.LCDC.flags.window_tile_map_select && !is_sprite){
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
            sprite_size = ppu.LCDC.flags.sprite_size ? 16 : 8;
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

    if(ppu.LCDC.flags.window_disp_enabled){
        //do something
        addr = 0;
    } else {
        //fetch bg tile number
        addr = ppu.LCDC.flags.bg_tile_map_select ? BACKGROUND2 : BACKGROUND1;
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
PPU_t* init_ppu(byte* perm_ptr){
    int fifo[2];
    pid_t pid = -1;
    if(pipe(fifo) == -1){
        LOG(ERROR, "Failed to create pipe for ppu and lcd");
        exit(1);
    }
    ppu.mem_perm_ptr = perm_ptr;
    *ppu.mem_perm_ptr = MEM_FREE;
    ppu.STAT.flags.PPU_mode = 1; //start in mode 0;
    ppu.dot_counter = 0; //can change later
    ppu.LY = 0x90; //start in vblank mode
#ifndef HEADLESS
    pid = fork();
    if(!pid){
        close(fifo[1]);
        init_lcd(fifo[0]);
        lcd_loop();
    } else {
#endif
        close(fifo[0]);
        ppu.LCD_fifo_write = fifo[1];
        ppu.lcd_pid = pid;
        ppu.STAT.flags.unused = 1; //must be set to one according to documentation
#ifndef HEADLESS
    }
#endif
    return &ppu;
}

void ppu_cycle(){
    ppu.dot_counter++; //used to keep track of progress internally
    switch(ppu.STAT.flags.PPU_mode){
        case HBLANK:
            if(ppu.dot_counter == DOTS_PER_SCANLINE){
                ppu.LY++;
                ppu.dot_counter = 0;
                if(ppu.LY == VBLANK_START){
                    ppu.STAT.flags.PPU_mode = VBLANK;
                    if(ppu.STAT.flags.mode_1_int) ppu.stat_int();
                } else {
                    ppu.STAT.flags.PPU_mode = OAM_SCAN;
                    if(ppu.STAT.flags.mode_2_int) ppu.stat_int();
                }
            }
            break;
        case VBLANK:
            if(ppu.dot_counter == DOTS_PER_SCANLINE){
                ppu.LY++;
                ppu.dot_counter = 0;
                if(ppu.LY == VBLANK_END){
                    ppu.LY = 0;
                    *ppu.mem_perm_ptr = OAM_BLOCKED;
                    ppu.STAT.flags.PPU_mode = OAM_SCAN;
                    if(ppu.STAT.flags.mode_2_int) ppu.stat_int();
                }
            }
            break;
        case OAM_SCAN:
            //last cycle of OAM SCAN
            if(ppu.dot_counter + 1 == DRAW_START){
                ppu.STAT.flags.PPU_mode = DRAW;
                *ppu.mem_perm_ptr = OAM_VRAM_BLOCKED;
            }
            break;
        case DRAW:
            //in reality the size of DRAW will very
            if(ppu.dot_counter + 1 == 0x100){
                ppu.STAT.flags.PPU_mode = HBLANK;
                if(ppu.STAT.flags.mode_0_int) ppu.stat_int();
                *ppu.mem_perm_ptr = MEM_FREE;
            }
            break;
        default:
            LOG(ERROR, "Incorrect ppu mode detected");
            exit(1);
            break;
    }
    if(ppu.LY == ppu.LYC){
        ppu.STAT.flags.coincidence_flag = 1;
        if(ppu.STAT.flags.LYC_stat_int) ppu.stat_int();
    }
    return;
}

int cleanup_ppu(){
    close(ppu.LCD_fifo_write);
    return 0;
}
