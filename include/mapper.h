#ifndef MAPPER_H
#define MAPPER_H
#include "common.h"
#include "main_bus.h"
#include <stdint.h>

/*
 * my idea for mapper is to abstract away the details by allowing a set of functions to be selected
 */

//depending on the type of the mapper, the functions will change. my idea is similar to f_ops from linux kernel
typedef struct mapper_struct{
    main_bus_t *bus; //backwards pointer to the bus, which will contain the pointers to RAM and ROM
    uint8_t (*read_ROM)(uint16_t addr);
    uint8_t (*read_RAM)(uint16_t addr);
    uint8_t (*write_RAM)(uint16_t addr);
} mapper_t;

mapper_t* create_mapper(main_bus_t* bus);

#endif // !MAPPER_H
