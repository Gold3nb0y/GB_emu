#include "emulator.h"
#include "main_bus.h"
#include "mapper.h"

static void cleanup(){
    LOG(INFO, "cleanup");
    release_bus(&emu.bus);  
    free(emu.mapper);
    exit(0);
}

void create_emulator(char* filename){
    //deref has higher precidence
    load_cart(&emu.cart, filename);
    create_bus(&emu.bus, emu.cart.RAM_size, emu.cart.ROM_size);
    create_mapper(&emu.bus);
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

