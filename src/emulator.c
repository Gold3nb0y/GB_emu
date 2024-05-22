#include "emulator.h"
#include "lcd.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

static emulator_t emu;

void cleanup(int sig){
    LOG(INFO, "cleanup");
    release_bus(emu.main_bus);  
    cleanup_ppu();
    // exit cleanly
    exit(0);
    return;
}

byte read_IF(void* io_reg);
void write_IF(void* io_reg, byte data);
byte read_IE(void* io_reg);
void write_IE(void* io_reg, byte data);

//TODO reset more thouroughly right now just setting things up for a new rom
void reset_cpu(){
    emu.main_bus->ROM_B0 = emu.main_bus->mapper->ROM_banks[0];
    emu.main_bus->ROM_BN = emu.main_bus->mapper->ROM_banks[1];
    memset(emu.cpu, 0, sizeof(CPU_t));
    emu.cpu->bus = emu.main_bus;
    cpu.SP = 0xFFFE;
    cpu.PC = 0x100;
    return;
}

void init_io_reg(io_reg *reg, address addr, read_io read_func, write_io write_func, bool readable, bool writeable){
    reg->storage = 0;
    reg->addr = addr;
    reg->read_callback = read_func;
    reg->write_callback = write_func;
    reg->readable = readable;
    reg->writeable = writeable;
    return;
}

uint64_t init_io(main_bus_t *bus){
    io_reg* ret;
    uint64_t i = 0;
    ret = calloc(NUM_REGS, sizeof(io_reg));
    if(!ret){
        LOG(ERROR, "calloc");
        exit(1);
    }
    
    //joypad is special since it is only written to by hardware.
    init_io_reg(&ret[i++], JOYP, read_joycon, write_joycon, true, true);
    init_io_reg(&ret[i++], SB, read_SB, write_SB, true, true);
    init_io_reg(&ret[i++], SC, NULL, write_SC, false, true);
    init_io_reg(&ret[i++], DIV, NULL, NULL, true, true);

    //timer stuff, seems complicated ignoring until I am sure I need a timer
    //init_io_reg(&ret[i++], TIMA, NULL, NULL, true, true);
    //init_io_reg(&ret[i++], TMA, NULL, NULL, true, true);
    //init_io_reg(&ret[i++], TAC, NULL, NULL, true, true);

    init_io_reg(&ret[i++], IF, read_IF, write_IF, true, true);
    //all the registers for the display
    init_io_reg(&ret[i++], LCDC, read_LCDC, write_LCDC, true, true);
    init_io_reg(&ret[i++], STAT, read_STAT, write_STAT, true, true);
    init_io_reg(&ret[i++], SCX, read_SCX, write_SCX, true, true);
    init_io_reg(&ret[i++], SCY, read_SCY, write_SCY, true, true);
    init_io_reg(&ret[i++], WX, read_WX, write_WX, true, true);
    init_io_reg(&ret[i++], WY, read_WY, write_WY, true, true);
    init_io_reg(&ret[i++], LY, read_LY, NULL, true, false);
    init_io_reg(&ret[i++], LYC, read_LYC, write_LYC, true, true);

    init_io_reg(&ret[i++], VBK, read_VBK, write_VBK, true, true);
    init_io_reg(&ret[i++], DMA, NULL, start_DMA, false, true);

    init_io_reg(&ret[i++], IE, read_IE, write_IE, true, true);

    bus->io_regs = ret;

    return i;
}

void setup_io_regs(){
    emu.main_bus->mapper->num_regs = init_io(emu.main_bus);
}

void create_emulator(char* filename){
    //deref has higher precidence
    bool is_CGB;
    signal(SIGINT, cleanup);
    load_cart(&emu.cart, filename);
    is_CGB = emu.cart.CGB_flag == 0x80 || emu.cart.CGB_flag == 0xC0;
    emu.main_bus = create_bus(emu.cart.num_ROM, emu.cart.val_RAM, is_CGB, filename);
    select_mapper(emu.cart.cart_type, emu.main_bus->mapper);
    emu.cpu = init_cpu(emu.main_bus);
    emu.ppu = init_ppu(&emu.main_bus->mem_perms);
    //sleep(2); //give the ppu process so time to start up
    setup_io_regs();
    emu.running = true;
    return;
}


void run(){
    uint64_t ticks, ticked;
    char check;
    ticks = 0;
    LOG(INFO, "Beginning ROM execution");
    while(emu.running){
#ifndef NATTACH_DB
        start_debugger(emu.main_bus, emu.cpu);
#else
        if((ticked = exec_program(4)) == 0) emu.running = false; //trigger cpu
        ticks += ticked;
        ticked *= 2;
        if(emu.main_bus->DMA_info.DMA_enabled){
            for(uint64_t i = 0; i < ticked; i++)
                DMA_tick();
        }
#endif
        //if(ticks % 0x100 == 0){
        //    dump_cpu();
        //    check = getchar();
        //    if(check != '\n') break;
        //}
        //do some kind of interupt handling here after the PPU and CPU have been ticked
    }
    LOG(INFO, "Ending ROM execution");
    getchar();
    cleanup(0);
}

byte read_IF(void* io_reg){
    return emu.IF;
}

void write_IF(void* io_reg, byte data){
    emu.IF = data;
}

byte read_IE(void* io_reg){
    return emu.IE;
}

void write_IE(void* io_reg, byte data){
    emu.IE = data;
}

emulator_t* get_emu(){
    return &emu;
}

#ifdef TEST
test_func tests[] = {
    test_ld,
    test_mem,
    test_arith,
    push_pop,
    call_ret,
    prefixed_instr,
    misc_instr,
    jumps,
    NULL,
};

void test_cpu(){
    uint64_t rc, i;
    LOG(INFO, "running tests");
    for(i = 0; tests[i] != NULL; i++){
        rc = tests[i]();
        if(rc){
            LOGF(ERROR, "Test %ld failed with rc %ld", i+1, rc);
            break;
        } else {
            LOGF(INFO, "Test %ld passed", i+1);
        }
        reset_cpu();
    }
    getchar();
    cleanup(0);
}
#else
void test_cpu(){
    LOG(ERROR, "Testing disabled");
}
#endif
