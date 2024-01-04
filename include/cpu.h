#ifndef CPU_H
#define CPU_H
#include "common.h"
#include "opcodes.h"
#include "main_bus.h"
#include "log.h"
#include <stdint.h>
#include <sys/types.h>

typedef uint16_t address;
typedef uint8_t byte;

#define DEBUG_CPU

//note to self since cpu uses the main bus, I can not use the CPU inside of main bus
typedef struct CPU_struct{
    main_bus_t* bus;
    union{
        uint16_t AF;
        struct {
            uint8_t F;
            uint8_t A;
        };
    };
    union{
        uint16_t BC;
        struct {
            uint8_t C;
            uint8_t B;
        };
    };
    union{
        uint16_t DE;
        struct {
            uint8_t E;
            uint8_t D;
        };
    };
    union{
        uint16_t HL;
        struct {
            uint8_t L;
            uint8_t H;
        };
    };
    uint16_t SP;
    uint16_t PC;
    byte IME; //interrupts enabled
}CPU_t;

static CPU_t cpu;

CPU_t* init(address entry, main_bus_t* bus);

void reset();

uint64_t exec(uint64_t ticks);
static void ld_rr(byte opcode);
static void ld_8bit_immi(byte opcode);
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg);
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg);
//static void setZ();
//static void clrZ();
//static void setS();
//static void clrS();
//static void setHC();
//static void clrHC();
//static void setC();
//static void clrC();
#endif
