#include "main_bus.h"
#include "log.h"
#include <stdlib.h>

static void* Malloc(ssize_t size){
    void* ret = malloc(size);
    if(!ret){
        LOG(ERROR, "failed to allocated RAM");
        exit(1);
    }
    return ret;
}

void create_bus(main_bus_t* bus, uint8_t RAM_size, uint8_t ROM_size){
    char* message = malloc(0x30);
    bus->ROM = malloc(ROM_SIZE * (1 << ROM_size));
    if(!bus->ROM){
        LOG(ERROR, "failed to allocate ROM");
        exit(1);
    }
    LOGF(DEBUG, "ROM: %p",bus->ROM);
    

    void* RAM;
    switch(RAM_size){
        case(2):
            RAM = Malloc(0x2000);
            break;
        case(3):
            RAM = Malloc(0x8000);
            break;
        case(4):
            RAM = Malloc(0x20000);
            break;
        case(5):
            RAM = Malloc(0x10000);
            break;
        default: //No RAM
            RAM = NULL;
            break;
    }
    bus->RAM = RAM;
    if(bus->RAM){
        LOGF(DEBUG, "RAM: %p",bus->RAM);
    } else {
        LOG(DEBUG, "RAM not used");
    }
    free(message);
    return;
}

void release_bus(main_bus_t* bus){
    free(bus->ROM);
    free(bus->RAM);
    return;
}
