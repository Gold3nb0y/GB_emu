#include "main_bus.h"
#include <stdint.h>


static uint8_t parse_ram(uint8_t header_val){
    switch(header_val){
        case 0:
            return 0;
        case 2:
            return 1;
        case 3:
            return 4;
        case 4:
            return 16;
        case 5:
            return 8;
        default:
            LOG(ERROR, "unused RAM value in header");
            exit(1);
    }
}

/*
 * bus: pointer to the address of bus from within the emulator struct
 * num_ROM: the value containing the num of ROM sections in the cartridge
 * val_RAM: value to be parsed into WorkerRam and VRAM chunks
 */
void create_bus(main_bus_t* bus, uint8_t num_ROM, uint8_t val_RAM, bool is_CGB){
    uint8_t num_VRAM, num_EXRAM, num_WRAM;
    if(is_CGB){
        num_VRAM = 2;
        num_WRAM = 8;
    } else {
        num_VRAM = 1;
        num_WRAM = 2;
    }
    num_EXRAM = parse_ram(val_RAM);
    bus->mapper = create_mapper(num_ROM, num_VRAM, num_EXRAM, num_WRAM); 
    //init values for the bus, later I may want to place these inside of the mapper, but not sure yet
    bus->ROM_B0 = bus->mapper->ROM_banks[0];
    bus->ROM_BN = bus->mapper->ROM_banks[1];
    bus->VRAM = bus->mapper->VRAM_banks[0];
    if(bus->mapper->EXRAM_banks)
        bus->EXRAM = bus->mapper->EXRAM_banks[0];
    else
        bus->EXRAM = NULL;
    bus->WRAM_B0 = bus->mapper->WRAM_banks[0];
    bus->WRAM_BN = bus->mapper->WRAM_banks[1];

    bus->OAM = Malloc(0xa0);
    bus->HRAM = Malloc(0x7F);
    bus->IE = 0xff; //still not sure of the init value
}

void release_bus(main_bus_t* bus){
    release_mapper(bus->mapper);
    free(bus->mapper);
    free(bus->OAM);
    free(bus->HRAM);
    memset(bus, 0, sizeof(main_bus_t));
    return;
}

uint8_t readBus(uint16_t addr){
    uint8_t ret = 0;
    if(addr < 0x4000){ //rom bank 00
        ;
    } else if (addr < 0x8000) { //rom bank 01, swichable via mapper
        ;
    } else if (addr < 0xA000) { //vram, switchable if CGB
        ;
    } else if (addr < 0xC000) { //external RAM
        ;
    } else if (addr < 0xD000) { //work RAM
        ;
    } else if (addr < 0xE000) { //work RAM2, switchable in CGB
        ;
    } else if (addr < 0xFE00) { //RAM mirror
        ;
    } else if (addr < 0xFEA0) { //Object attribue memory
        ;
    } else if (addr < 0xFF00) { //unusable
        ;
    } else if (addr < 0xFF80) { //i/o registers
        ;
    } else if (addr < 0xFFFF) { //high ram
        ;
    } else if (addr == 0xFFFF) { //Interupt enable register
        ;
    }
    return ret;
}

uint8_t writeBus(uint16_t addr){
    if(addr < 0x4000){ //rom bank 00
        ;
    } else if (addr < 0x8000) { //rom bank 01, swichable via mapper
        ;
    } else if (addr < 0xA000) { //vram, switchable if CGB
        ;
    } else if (addr < 0xC000) { //external RAM
        ;
    } else if (addr < 0xD000) { //work RAM
        ;
    } else if (addr < 0xE000) { //work RAM2, switchable in CGB
        ;
    } else if (addr < 0xFE00) { //RAM mirror
        ;
    } else if (addr < 0xFEA0) { //Object attribue memory
        ;
    } else if (addr < 0xFF00) { //unusable
        ;
    } else if (addr < 0xFF80) { //i/o registers
        ;
    } else if (addr < 0xFFFF) { //high ram
        ;
    } else if (addr == 0xFFFF) { //Interupt enable register
        ;
    }
    return 0;
}
