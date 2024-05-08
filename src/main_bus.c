#include "main_bus.h"
#include <stdint.h>

static main_bus_t* bus;

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
main_bus_t* create_bus(uint8_t num_ROM, uint8_t val_RAM, bool is_CGB, char* filename){
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
    bus->mapper = create_mapper(num_ROM, num_VRAM, num_EXRAM, num_WRAM, filename); 
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
    bus->IE = 0xff; //still not sure of the init value
    return bus;
}

void release_bus(main_bus_t* bus){
    release_mapper(bus->mapper);
    free(bus->mapper);
    free(bus->OAM);
    memset(bus, 0, sizeof(main_bus_t));
    free(bus);
    LOG(INFO, "bus freed")
    return;
}

int check_io_reg(address addr, io_reg* regs){
    int ret = 0;
    for(;ret < bus->mapper->num_regs; ret++){
        if(regs[ret].addr == addr)
            goto success;
    }
    ret = 1;
success:
    return ret;
}

byte read_bus_generic(address addr){
    byte ret = 0;
    if(addr >= VRAM_START && addr < EXRAM_START){
        ret = bus->mapper->VRAM_banks[bus->mapper->cur_VRAM][addr-VRAM_START];
    } else if(addr >= WRAM0_START && addr < WRAMN_START){
        ret = bus->mapper->WRAM_banks[0][addr-WRAM0_START];
    } else if(addr >= WRAMN_START && addr < WRAMN_END){
        ret = bus->mapper->WRAM_banks[bus->mapper->cur_WRAM][addr-WRAMN_START];
    } else if(addr >= WRAMN_END && addr < OAM_START){
        LOG(ERROR, "undocumented memory access");
    } else if(addr >= OAM_START && addr < OAM_END){
        LOG(ERROR, "unimplemented");
    } else if(addr >= OAM_END && addr < IO_START){
        LOG(ERROR, "undocumented memory access");
    } else if(addr >= IO_START && addr < HRAM_START){
        int idx = check_io_reg(addr, bus->mapper->io_regs);
        if(idx == -1){
            LOG(ERROR, "register not mapper");
            return 0;
        }
        ret = bus->mapper->io_regs[idx].callback(0);
    } else if(addr >= HRAM_START && addr < IE_REG){
        ret = bus->mapper->HRAM[addr-HRAM_START];
    } else {
        //Interupt enable
        LOG(ERROR, "unimplemented");
    }
    return ret;
}

void write_bus_generic(address addr, byte data){
    if(addr >= VRAM_START && addr < EXRAM_START){
        bus->mapper->VRAM_banks[bus->mapper->cur_VRAM][addr-VRAM_START] = data;
    } else if(addr >= WRAM0_START && addr < WRAMN_START){
        bus->mapper->WRAM_banks[0][addr-WRAM0_START] = data;
    } else if(addr >= WRAMN_START && addr < WRAMN_END){
        bus->mapper->WRAM_banks[bus->mapper->cur_WRAM][addr-WRAMN_START] = data;
    } else if(addr >= WRAMN_END && addr < OAM_START){
        LOG(ERROR, "undocumented memory access");
    } else if(addr >= OAM_START && addr < OAM_END){
        LOG(ERROR, "unimplemented");
    } else if(addr >= OAM_END && addr < IO_START){
        LOG(ERROR, "undocumented memory access");
    } else if(addr >= IO_START && addr < HRAM_START){
        int idx = check_io_reg(addr, bus->mapper->io_regs);
        if(idx == -1){
            LOG(ERROR, "register not mapper");
            return;
        }
        bus->mapper->io_regs[idx].callback(data);
    } else if(addr >= HRAM_START && addr < IE_REG){
        bus->mapper->HRAM[addr-HRAM_START] = data;
    } else {
        //Interupt enable
        LOG(ERROR, "unimplemented");
    }
}

//trigger any special addresses and if there are none read from mapper
byte read_bus(address addr){
#ifdef DEBUG_BUS
    LOGF(DEBUG, "attempting to read addr: 0x%x",addr);
#endif
    byte ret;
    if((addr >= 0x8000 && addr < 0xA000) || addr >= 0xC000){
        ret = read_bus_generic(addr);
    } else {
        ret = bus->mapper->read(addr);
    }
    return ret;
}

address read_bus_addr(address addr){
    address ret;
    ret = read_bus(addr) << 8;
    ret |= read_bus(addr+1);
    return ret;
}

void write_bus(address addr, byte chr){
#ifdef DEBUG_BUS
    LOGF(DEBUG, "Writing 0x%x to 0x%x", chr, addr);
#endif
    if((addr >= 0x8000 && addr < 0xA000) || addr >= 0xC000){
        write_bus_generic(addr, chr);
    } else {
        bus->mapper->write(addr, chr);
    }
    return;
}

void write_bus_addr(address dest, address addr){
    write_bus(dest, addr >> 8);
    write_bus(dest+1, addr & 0xff);
    return;
}
