#ifndef MAIN_BUS_H
#define MAIN_BUS_H
#include "common.h"
#include "mapper.h"
#include <stdint.h>

//filled during mapper creation. mapper will handle swapping
typedef struct main_bus_struct {
    byte* ROM_B0;
    byte* ROM_BN; //switchable rom bank
    byte* VRAM;
    byte* EXRAM;
    byte* WRAM_B0;
    byte* WRAM_BN; //CGB switchable 1-7
    byte* OAM;
    byte* HRAM;
    mapper_t* mapper;
    byte  IE;
}main_bus_t;

static main_bus_t* bus;

main_bus_t* create_bus(uint8_t num_ROM, uint8_t val_RAM, bool is_CGB);
void release_bus(main_bus_t* bus);
byte read_bus(address addr);
void write_bus(address addr, byte chr);
address read_bus_addr(address addr);
void write_bus_addr(address dest, address addr);

#endif
