#include "emulator.h"
#include <stdint.h>
#include <sys/types.h>

static emulator_t emu;

static void cleanup(){
    LOG(INFO, "cleanup");
    release_bus(emu.main_bus);  
    exit(0);
}

//TODO reset more thouroughly right now just setting things up for a new rom
void reset_cpu(){
    emu.main_bus->ROM_B0 = emu.main_bus->mapper->ROM_banks[0];
    emu.main_bus->ROM_BN = emu.main_bus->mapper->ROM_banks[1];
    memset(emu.cpu + 8, 0, sizeof(CPU_t)-8);
    cpu.SP = 0xFFFE;
    cpu.PC = 0x100;
    return;
}

void create_emulator(char* filename){
    //deref has higher precidence
    bool is_CGB;
    load_cart(&emu.cart, filename);
    is_CGB = emu.cart.CGB_flag == 0x80 || emu.cart.CGB_flag == 0xC0;
    emu.main_bus = create_bus(emu.cart.num_ROM, emu.cart.val_RAM, is_CGB, filename);
    select_mapper(emu.cart.cart_type, emu.main_bus->mapper);
    emu.cpu = init(emu.main_bus);
    emu.running = true;
    return;
}

void run(){
    uint64_t ticks, ticked;
    ticks = 0;
    LOG(INFO, "Beginning ROM execution");
    while(emu.running){
        if((ticked = exec_program(4)) == 0) emu.running = false; //trigger cpu
        ticks += ticked;
        if(ticks > 0x10)
            break;
    }
    cleanup();
}


emulator_t* get_emu(){
    return &emu;
}

#ifdef TEST
test_func tests[] = {
    test1,
    test2,
    test3,
    test4,
    test5,
};

void test_cpu(){
    uint64_t rc, i;
    LOG(INFO, "running tests");
    for(i = 0; i < NUM_TESTS; i++){
        rc = tests[i]();
        if(rc){
            LOGF(ERROR, "Test %ld failed with rc %ld", i, rc);
        } else {
            LOGF(INFO, "Test %ld passed", i);
        }
        reset_cpu();
    }
}
#else
void test_cpu(){
    LOG(ERROR, "Testing disabled");
}
#endif
