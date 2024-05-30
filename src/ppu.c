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

byte read_BGP(){
    return ppu.BGP;
}

void write_BGP(byte data){
    ppu.BGP = data;
}

byte read_OBP0(){
    return ppu.OBP0;
}

void write_OBP0(byte data){
    ppu.OBP0 = data & 0xFC;
}

byte read_OBP1(){
    return ppu.OBP1;
}

void write_OBP1(byte data){
    ppu.OBP1 = data & 0xFC;
}

void read_obj(uint8_t idx, obj_t *obj){
    obj->Y = read_bus(OAM_START + idx * sizeof(obj_t));
    obj->X = read_bus(OAM_START + idx * sizeof(obj_t)+1);
    obj->tile_index = read_bus(OAM_START + idx * sizeof(obj_t)+2);
    obj->sprite_flag = read_bus(OAM_START + idx * sizeof(obj_t)+3);
    return;
}

struct sprite_stack {
    obj_t to_display[10]; //up to 10 spt_metadata can be displayed for any 1 scanline
    uint8_t count;
    uint8_t current;
};

struct sprite_stack spt_metadata = {0};

//takes 80 cycles
void scan_OAM(uint16_t scanline){
    uint8_t count = 0;
    obj_t tmp_obj = {0};
    uint8_t sprite_size = 0;
    memset(&spt_metadata, 0, sizeof(struct sprite_stack)); //reset the displaybuffer
    for(uint8_t i = 0; i < 40; i++){
        read_obj(i, &tmp_obj);
        if(tmp_obj.X > 0 && scanline+16 >= tmp_obj.Y && count < 10){
            sprite_size = ppu.LCDC.flags.sprite_size ? 16 : 8;
            if(scanline+16 < tmp_obj.Y+sprite_size){
                memcpy(&spt_metadata.to_display[count], &tmp_obj, sizeof(obj_t));
                count++;
            }
        }
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
    if(fcntl(fifo[0], F_SETFL, O_NONBLOCK) == -1){
        LOG(ERROR, "Failed to make pipe non blocking");
        exit(1);
    }
    if(fcntl(fifo[1], F_SETFL, O_NONBLOCK) == -1){
        LOG(ERROR, "Failed to make pipe non blocking");
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
        close(fifo[0]);
#endif
        ppu.LCD_fifo_write = fifo[1];
        ppu.lcd_pid = pid;
        ppu.STAT.flags.unused = 1; //must be set to one according to documentation
#ifndef HEADLESS
    }
#endif
    return &ppu;
}

void read_tile_row(uint8_t tile_idx, uint8_t row_num, uint8_t type, uint8_t *first, uint8_t *second){
    address addr;

    if(type != OBJ && ppu.LCDC.flags.tile_data_select == 0){
        addr = 0x9000;
        addr += (int8_t)tile_idx * 0x10;
        //printf("addr of tile row 0x%x\n", addr);
    } else {
        addr = 0x8000;
        addr += tile_idx * 0x10;
    }

    addr += row_num * 2; //2 bytes per row
    
    printf("data addr: 0x%04x\n", addr);
    *first = read_bus(addr);
    *second = read_bus(addr+1);
}

//TODO worry about window drawing
void draw_line(){
    uint8_t tile_idx, x, y, x_off, y_off, i, j;
    uint8_t first, second;
    uint16_t row, tile_map, addr, count;
    scanline line;
    obj_t tmp_spt;

    x = ppu.SCX;
    //x = (count % 256) / 8;
    x_off = ppu.SCX % 8;
    count = ppu.LY;
    count += ppu.SCY;
    //printf("LY 0x%x SCY 0x%x\n", ppu.LY, ppu.SCY);
    y = (count % 256) / 8;
    count = ppu.SCY + ppu.LY;
    y_off = count % 8;

    //printf("y_val 0x%x\n", y);

    tile_map = ppu.LCDC.flags.bg_tile_map_select ? 0x9C00 : 0x9800;

    memset(&line, 0, sizeof(scanline));

    //x will overflow to 0 and wrap around
    addr = tile_map + (y * 32);
    for(i = 0; i < 20; i++){
        tile_idx = read_bus(addr + (x / 8));
        printf("addr: 0x%04x x: %02d y: %02d y_off: %d tile_idx: 0x%02x ", addr, x, y, y_off, tile_idx);
        read_tile_row(tile_idx, y_off, BG, &first, &second);
        for(j = x_off; j < 8; j++){
            line.bg_pixels[count++] = first >> (7 - j) & 1;
        }
        x++;
    }

#ifndef BG_ONLY
    //write all of the sprite data into an aligned array
    for(i = 0; i < spt_metadata.count; i++){
        memcpy(&tmp_spt, &spt_metadata.to_display[i], sizeof(obj_t));
        y_off = tmp_spt.flags.Y_flip ? ~(tmp_spt.Y % 8) : tmp_spt.Y % 8;
        line.spt_data[i].sprite_row = read_tile_row(tmp_spt.tile_index, y_off, OBJ);
        line.spt_data[i].pallette = tmp_spt.flags.DMG_pallette ? ppu.OBP1 : ppu.OBP0;
        line.spt_data[i].X = tmp_spt.X;
        line.spt_data[i].sprite_flag = tmp_spt.sprite_flag;
    }
#endif

    line.bg_to_obj = ppu.LCDC.flags.window_disp_enabled ? true : false; //only useful for cgb
    line.BGP = ppu.BGP;
    line.num_spt = spt_metadata.count;

    write(ppu.LCD_fifo_write, &line, sizeof(scanline));
    return;
}

//TODO rework the stat editor
void ppu_cycle(){
    ppu.dot_counter++; //used to keep track of progress internally
    switch(ppu.STAT.flags.PPU_mode){
        case HBLANK:
            if(ppu.dot_counter == DOTS_PER_SCANLINE){
                ppu.LY++;
                ppu.dot_counter = 0;
                if(ppu.LY == VBLANK_START){
                    ppu.STAT.flags.PPU_mode = VBLANK;
                    ppu.vblank_int();
                    if(ppu.STAT.flags.mode_1_int) ppu.stat_int();
                } else {
                    ppu.STAT.flags.PPU_mode = OAM_SCAN;
                    if(ppu.STAT.flags.mode_2_int) ppu.stat_int();
                }
            } else {
                //hblank logic
            }
            break;
        case VBLANK:
            if(ppu.dot_counter == DOTS_PER_SCANLINE){
                ppu.LY++;
                ppu.dot_counter = 0;
                if(ppu.LY == VBLANK_END){
                    ppu.LY = 0;
                    //set up the parameters for drawing
                    *ppu.mem_perm_ptr = OAM_BLOCKED;
                    ppu.STAT.flags.PPU_mode = OAM_SCAN;
                    if(ppu.STAT.flags.mode_2_int) ppu.stat_int();
                }
            }
            //do nothing
            break;
        case OAM_SCAN:
            //last cycle of OAM SCAN
            if(ppu.dot_counter + 1 == DRAW_START){
                //I'll scan OAM all at once when it's cycle is finished, should save time in overhead
                scan_OAM(ppu.LY);
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
                draw_line();
                if(ppu.LY == 0) getchar();
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
