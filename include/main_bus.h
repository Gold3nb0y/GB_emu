#ifndef MAIN_BUS_H
#define MAIN_BUS_H
#include "common.h"
#include <stdint.h>
#define ROM_SIZE 0x8000

typedef struct main_bus_struct {
    uint8_t* ROM;
    uint8_t* RAM;
}main_bus_t;

void create_bus(main_bus_t* bus, uint8_t RAM_size, uint8_t ROM_size);
void release_bus(main_bus_t* bus);

#endif
