#include "emulator.h"
#include "main_bus.h"
#include "mapper.h"

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
    emu.running = true;
    return;
}

void run(){
    uint64_t tick = 0;
    LOG(INFO, "Beginning ROM execution");
    while(emu.running){
        if(tick > 0x1000) emu.running = false;
        tick++;
    }
    cleanup();
}

emulator_t* get_emu(){
    return &emu;
}

