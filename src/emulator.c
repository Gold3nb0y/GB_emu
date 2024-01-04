#include "emulator.h"
#include <stdint.h>

static void cleanup(){
    LOG(INFO, "cleanup");
    release_bus(&emu.bus);  
    exit(0);
}

void create_emulator(char* filename){
    //deref has higher precidence
    bool is_CGB;
    load_cart(&emu.cart, filename);
    is_CGB = emu.cart.CGB_flag == 0x80 || emu.cart.CGB_flag == 0xC0;
    create_bus(&emu.bus, emu.cart.num_ROM, emu.cart.val_RAM, is_CGB);
    emu.cpu = init(emu.cart.entry, &emu.bus);
    emu.running = true;
    return;
}

void run(){
    uint64_t ticks, ticked;
    ticks = 0;
    LOG(INFO, "Beginning ROM execution");
    while(emu.running){
        if((ticked = exec(4)) == 0) emu.running = false; //trigger cpu
        ticks += ticked;
    }
    cleanup();
}

emulator_t* get_emu(){
    return &emu;
}

