#ifndef MAIN_BUS_H
#define MAIN_BUS_H
#include "common.h"
#include "mapper.h"
#include <stdint.h>

//filled during mapper creation. mapper will handle swapping
typedef struct main_bus_struct {
    uint8_t* ROM_B0;
    uint8_t* ROM_BN; //switchable rom bank
    uint8_t* VRAM;
    uint8_t* EXRAM;
    uint8_t* WRAM_B0;
    uint8_t* WRAM_BN; //CGB switchable 1-7
    uint8_t* OAM;
    uint8_t* HRAM;
    mapper_t* mapper;
    uint8_t  IE;
}main_bus_t;


void create_bus(main_bus_t* bus, uint8_t num_ROM, uint8_t val_RAM, bool is_CGB);
void release_bus(main_bus_t* bus);
uint8_t read(uint16_t addr);
uint8_t writeBus(uint16_t addr);

#endif
