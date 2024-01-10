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
    byte** ROM_banks;
    byte** VRAM_banks;
    byte** EXRAM_banks;
    byte** WRAM_banks;
    //define a read write interface
    byte (*read)(address addr);
    void (*write)(address addr, byte chr);
    byte num_ROM;
    byte num_VRAM;
    byte num_EXRAM;
    byte num_WRAM;
    byte cur_ROM;
    byte cur_VRAM;
    byte cur_EXRAM;
    byte cur_WRAM;
    union{
        struct {
            bool RAM_enabled;
            struct {
                unsigned reg1: 5;
                unsigned reg2: 2;
                unsigned banking_mode_select: 1;
            };
        } MCB1;
    };
}__attribute__((packed)) mapper_t;

mapper_t* create_mapper(uint8_t num_ROM, uint8_t num_VRAM, uint8_t num_EXRAM, uint8_t num_WRAM);
uint8_t* swap_ROM(uint8_t bank_num);
uint8_t* swap_VRAM(uint8_t bank_num);
uint8_t* swap_WRAM(uint8_t bank_num);
void release_mapper(mapper_t* mapper);
byte read_MBC1(address addr);
void write_MBC1(address addr, byte data);

//for internal use;
static mapper_t *map;

#endif // !MAPPER_H
