#include "mapper.h"

mapper_t* create_mapper(uint8_t num_ROM, uint8_t num_VRAM, uint8_t num_EXRAM, uint8_t num_WRAM){
    mapper_t* mapper;
    mapper = Malloc(sizeof(mapper_t));

    //setup the arrays to be used 
    mapper->ROM_banks = Malloc(sizeof(size_t)*(1<<num_ROM));

    //working with max case to start
    mapper->VRAM_banks = Malloc(sizeof(size_t)*2);
    if(num_EXRAM)
        mapper->EXRAM_banks = Malloc(sizeof(size_t)*num_EXRAM);
    else 
        mapper->EXRAM_banks = NULL;
    mapper->WRAM_banks = Malloc(sizeof(size_t)*8);

    //map continously for performance?
    mapper->ROM_banks[0] = Mmap(NULL, ROM_SIZE*(1<<num_ROM), PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANON, -1, 0);
    mapper->num_ROM = (1<<num_ROM);

    //populate the ROM_banks list. this will be used for swapping to the specific pages
    for(uint8_t i = 1; i < (1<<num_ROM); i++)
        mapper->ROM_banks[i] = mapper->ROM_banks[0] + ROM_SIZE*i;
    LOGF(DEBUG, "ROM: %p",mapper->ROM_banks[0]);

    //for now I will handle the maximal case only. I will handle other cases in the future
    mapper->VRAM_banks[0] = Mmap(NULL, RAM_SIZE*2, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANON, -1, 0);
    mapper->VRAM_banks[1] = mapper->VRAM_banks[0] + RAM_SIZE;
    LOGF(DEBUG, "VRAM: %p",mapper->VRAM_banks[0]);
    mapper->num_VRAM = num_VRAM;

    if(num_EXRAM){
        mapper->EXRAM_banks[0] = Mmap(NULL, RAM_SIZE*num_EXRAM, PROT_READ | PROT_WRITE, 
                MAP_PRIVATE | MAP_ANON, -1, 0);
        for(uint8_t i = 1; i < num_EXRAM; i++)
            mapper->ROM_banks[i] = mapper->EXRAM_banks[0] + RAM_SIZE*i;
        LOGF(DEBUG, "EXRAM: %p",mapper->EXRAM_banks[0]);
    } else {
        LOG(DEBUG, "no exram availible");
    }
    mapper->num_EXRAM = num_EXRAM;

    mapper->WRAM_banks[0] = Mmap(NULL, WRAM_SIZE*8, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE | MAP_ANON, -1, 0);
    for(uint8_t i = 1; i < 8; i++)
        mapper->WRAM_banks[i] = mapper->VRAM_banks[0] + WRAM_SIZE*i;
    mapper->num_WRAM = 8;
    if(mapper->WRAM_banks[0]){
        LOGF(DEBUG, "WRAM: %p",mapper->WRAM_banks[0]);
    } else {
        LOG(DEBUG, "WRAM not used");
    }

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
