#include "mapper.h"
#include <stdint.h>

static void map_region(uint8_t** bank, size_t size, uint8_t number){
    //map continously for performance?
    bank[0] = Mmap(NULL, size*number, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANON, -1, 0);

    //populate the ROM_banks list. this will be used for swapping to the specific pages
    for(uint8_t i = 1; i < number; i++)
        bank[i] = bank[0] + size*i;
}

mapper_t* create_mapper(uint8_t num_ROM, uint8_t num_VRAM, uint8_t num_EXRAM, uint8_t num_WRAM){
    mapper_t* mapper;
    mapper = Malloc(sizeof(mapper_t));

    //setup the arrays to be used 
    mapper->ROM_banks = Malloc(sizeof(size_t)*(1<<num_ROM));

    mapper->VRAM_banks = Malloc(sizeof(size_t)*num_VRAM);
    if(num_EXRAM)
        mapper->EXRAM_banks = Malloc(sizeof(size_t)*num_EXRAM);
    else 
        mapper->EXRAM_banks = NULL;
    mapper->WRAM_banks = Malloc(sizeof(size_t)*8);

    map_region(mapper->ROM_banks, ROM_SIZE, (1<<num_ROM));
    mapper->num_ROM = (1<<num_ROM);
    LOGF(DEBUG, "ROM: %p",mapper->ROM_banks[0]);

    //for now I will handle the maximal case only. I will handle other cases in the future
    map_region(mapper->VRAM_banks, RAM_SIZE, num_VRAM);
    mapper->num_VRAM = num_VRAM;
    LOGF(DEBUG, "VRAM: %p",mapper->VRAM_banks[0]);

    if(num_EXRAM){
        map_region(mapper->EXRAM_banks, RAM_SIZE, num_EXRAM);
        LOGF(DEBUG, "EXRAM: %p",mapper->EXRAM_banks[0]);
    } else {
        LOG(DEBUG, "no exram availible");
    }
    mapper->num_EXRAM = num_EXRAM;

    map_region(mapper->WRAM_banks, WRAM_SIZE, num_WRAM);
    mapper->num_WRAM = num_WRAM;
    LOGF(DEBUG, "WRAM: %p",mapper->WRAM_banks[0]);

    //to be figured out later
    mapper->read_RAM = NULL;
    mapper->read_ROM = NULL;
    mapper->write_RAM = NULL; 
    return mapper;
}

void release_mapper(mapper_t* mapper){
    munmap(mapper->ROM_banks[0], ROM_SIZE*mapper->num_ROM);
    munmap(mapper->VRAM_banks[0], RAM_SIZE*mapper->num_VRAM);
    munmap(mapper->WRAM_banks[0], WRAM_SIZE*mapper->num_WRAM);
    if(mapper->EXRAM_banks)
        munmap(mapper->WRAM_banks[0], WRAM_SIZE*mapper->num_WRAM);
    free(mapper->ROM_banks);
    free(mapper->VRAM_banks);
    free(mapper->WRAM_banks);
    memset(mapper, 0, sizeof(mapper_t));
    return;
}
