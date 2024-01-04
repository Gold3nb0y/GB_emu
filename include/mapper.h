#ifndef MAPPER_H
#define MAPPER_H
#include "common.h"
#include <stdint.h>
#include "log.h"

#define ROM_SIZE 0x4000
#define RAM_SIZE 0x2000
#define WRAM_SIZE 0x1000
/*
 * my idea for mapper is to abstract away the details by allowing a set of functions to be selected
 */

//depending on the type of the mapper, the functions will change. my idea is similar to f_ops from linux kernel
typedef struct mapper_struct{
    uint8_t** ROM_banks;
    uint8_t** VRAM_banks;
    uint8_t** EXRAM_banks;
    uint8_t** WRAM_banks;
    uint8_t (*read_ROM)(uint16_t addr);
    uint8_t (*read_RAM)(uint16_t addr);
    uint8_t (*write_RAM)(uint16_t addr);
    uint8_t num_ROM;
    uint8_t num_VRAM;
    uint8_t num_EXRAM;
    uint8_t num_WRAM;
} mapper_t;

mapper_t* create_mapper(uint8_t num_ROM, uint8_t num_VRAM, uint8_t num_EXRAM, uint8_t num_WRAM);
uint8_t* swap_ROM(uint8_t bank_num);
uint8_t* swap_VRAM(uint8_t bank_num);
uint8_t* swap_WRAM(uint8_t bank_num);
void release_mapper(mapper_t* mapper);

#endif // !MAPPER_H
