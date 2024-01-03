#ifndef EMU_H
#define EMU_H
#include "common.h"
#include "cpu.h"
#include "mapper.h"
#include "ppu.h"
#include "cart.h"
#include "main_bus.h"
#include "log.h"

typedef struct emulator_struct{
    CPU_t cpu;
    main_bus_t bus;
    PPU_t PPU;
    cart_t cart;
    mapper_t *mapper; //a refrence as the size may very depending on the type of mapper
    bool running;
}emulator_t;

static emulator_t emu;

//initialize all of the variable required to start. includes reading the cartridge header
void create_emulator(char* filename);
void run();
emulator_t* get_emu();
#endif
