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
            LOG(ERROR, "unused RAM value in header, returning 0");
            return 0;
    }
}

/*
 * num_ROM: the value containing the num of ROM sections in the cartridge
 * val_RAM: value to be parsed into WorkerRam and VRAM chunks
 */
main_bus_t* create_bus(uint8_t num_ROM, uint8_t val_RAM, bool is_CGB){
    bus = malloc(sizeof(main_bus_t));
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
    return bus;
}

void release_bus(main_bus_t* bus){
    release_mapper(bus->mapper);
    free(bus->mapper);
    free(bus->OAM);
    free(bus->HRAM);
    memset(bus, 0, sizeof(main_bus_t));
    free(bus);
    return;
}

//trigger any special addresses and if there are none read from mapper
byte read_bus(address addr){
    byte ret;
    ret = bus->mapper->read(addr);
    return ret;
}

<<<<<<< HEAD
address read_bus_addr(address addr){
    address ret;
    ret = bus->mapper->read(addr) << 8;
    ret |= bus->mapper->read(addr+1);
    return ret;
}

=======
>>>>>>> origin/master
void write_bus(address addr, byte chr){
    //the mapper will take control entirely at this point
    bus->mapper->write(addr, chr);
    return;
}
<<<<<<< HEAD

void write_bus_addr(address dest, address addr){
    //the mapper will take control entirely at this point
    bus->mapper->write(dest, addr >> 8);
    bus->mapper->write(dest+1, addr & 0xff);
    return;
}
=======
>>>>>>> origin/master
