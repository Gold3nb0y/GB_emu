#include "cpu.h"
#include "common.h"
#include "main_bus.h"
#include "mapper.h"
#include <stdint.h>
#include <sys/types.h>

static void ld_rr(byte opcode);
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg);
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg);

//for testing
static void patch(){
    ssize_t i = 0x1000;

    //hardcord in some test values;
    cpu.bus->ROM_B0[i++] = 0x0E;
    cpu.bus->ROM_B0[i++] = 0x69;
    cpu.bus->ROM_B0[i++] = 0x41;
    cpu.bus->ROM_B0[i++] = 0x50;
    cpu.bus->ROM_B0[i++] = 0x2E;
    cpu.bus->ROM_B0[i++] = 0x20;
    cpu.bus->ROM_B0[i++] = 0x26;
    cpu.bus->ROM_B0[i++] = 0x04;
    cpu.bus->ROM_B0[i++] = 0;
    cpu.bus->ROM_B0[i++] = 0x76; //HALT

    cpu.PC = 0x1000;
}

CPU_t* init(address entry, main_bus_t* bus){
    memset(&cpu, 0, sizeof(CPU_t));
    cpu.SP = 0x100;
    cpu.PC = entry;
    cpu.bus = bus;

#ifdef DEBUG_CPU
    patch();
#endif

    //give the emulator a refrence to the CPU
    return &cpu;
}

//handle instructions that only use 1 register
static void single_reg_inst(byte opcode){
    uint16_t *temp_reg16;
    uint8_t *temp_reg8;

    switch (opcode & 0xf) {
        case 1: //ld 16 bit constant
            get_16bit_register(opcode, 4, &temp_reg16);
            memcpy(temp_reg16, &cpu.bus->ROM_B0[cpu.PC], 2); 
            cpu.PC += 2;
            break;
        case 3: //inc 16 bit reg
            get_16bit_register(opcode, 4, &temp_reg16);
            *temp_reg16 += 1;
            break;
        case 9: //sum reg into HL
            get_16bit_register(opcode, 4, &temp_reg16);
            cpu.HL += *temp_reg16;
        case 0xB: //dec 16 bit reg
            get_16bit_register(opcode, 4, &temp_reg16);
            *temp_reg16 -= 1;
            break;
        case 4: //inc 8bit reg
        case 0xC:
            get_8bit_register(opcode, 3, &temp_reg8);
            *temp_reg8 += 1;
            break;
        case 5: //dec 8bit register
        case 0xD:
            get_8bit_register(opcode, 3, &temp_reg8);
            *temp_reg8 -= 1;
            break;
        case 6:  //ld 8 bit register immidiate
        case 0xE:
            get_8bit_register(opcode, 3, &temp_reg8);
            *temp_reg8 = cpu.bus->ROM_B0[cpu.PC]; //replace with read soon
            cpu.PC++;
            break;
        default:
            LOG(ERROR, "Not implemented");
    }
}
//TODO think about running instructions in parrellel with Fetch/execute overlap
//alternatively, I can also run in sequence, and code in the different stages later.
//basically, I could try to fetch something every cycle, this would involve maintaining a transitional state 
//Maybe I could break the instructions into tasklets that preform an operation each iteration?
//another thing I coudl do is the just estimate all the registers and grab the regardless of the instruction
uint64_t exec(uint64_t ticks){
    byte opcode;

    for(uint64_t i = 0; i < ticks; i++){
        opcode = cpu.bus->ROM_B0[cpu.PC];
        cpu.PC++;
#ifdef DEBUG_CPU
        LOG(DEBUG,"---CPU contents---");
        LOGF(DEBUG,"OPCODE: 0x%02x",opcode);
        LOGF(DEBUG,"A: 0x%02x F:0x%02x",cpu.A,cpu.F);
        LOGF(DEBUG,"BC: 0x%04x",cpu.BC);
        LOGF(DEBUG,"DE: 0x%04x",cpu.DE);
        LOGF(DEBUG,"HL: 0x%04x",cpu.HL);
        LOGF(DEBUG,"PC: 0x%04x",cpu.PC);
        LOGF(DEBUG,"PC: 0x%04x",cpu.SP);
        LOG(DEBUG,"------------------");
#endif
        switch(opcode >> 6){
            case 0:
                single_reg_inst(opcode);
                break;
            case 1: //register loads
                if(opcode == HALT) return 0; //stop the system clock
                ld_rr(opcode);
                break;
            case 2: //arithmetic and bitwise operations
                cpu.D = cpu.C;
                break;
            case 3: //control flow

            default:
                LOGF(ERROR,"unsupported instruction: 0x%x", opcode);
                return 0;
        }
    }
    return ticks;
}

//register are stored as 3 bit values in the opcode, offset if the number of bits to right shift
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg){
    switch(opcode >> offset & 0x7){
        case 0:
            *reg = &cpu.B;
            break;
        case 1:
            *reg = &cpu.C;
            break;
        case 2:
            *reg = &cpu.D;
            break;
        case 3:
            *reg = &cpu.E;
            break;
        case 4:
            *reg = &cpu.H;
            break;
        case 5:
            *reg = &cpu.L;
            break;
        case 7:
            *reg = &cpu.A;
            break;
        default:
            //plan is to return a refrence to the memory address
            LOG(ERROR, "memory addressing not yet implemented");
            break;
    }
}

//register are stored as 3 bit values in the opcode, offset if the number of bits to right shift
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg){
    switch(opcode >> offset & 0x3){
        case 0:
            *reg = &cpu.BC;
            break;
        case 1:
            *reg = &cpu.DE;
            break;
        case 2:
            *reg = &cpu.HL;
            break;
        case 3:
            *reg = &cpu.SP;
            break;
    }
}

//register to register or memory to register load
static void ld_rr(byte opcode){
    uint8_t *dest, *src;
    get_8bit_register(opcode, 3, &dest);
    get_8bit_register(opcode, 0, &src);
    *dest = *src;
}

//static void setZ(){
//    cpu.AF |= 1<<7;
//}
//
//static void clrZ(){
//    cpu.AF &= ~(1<<7);
//}
//
//static void setS(){
//    cpu.AF |= 1<<6;
//}
//
//static void clrS(){
//    cpu.AF &= ~(1<<6);
//}
//
////HC = half carry
//static void setHC(){
//    cpu.AF |= 1<<5;
//}
//
//static void clrHC(){
//    cpu.AF &= ~(1<<5);
//}
//
////HC = half carry
//static void setC(){
//    cpu.AF |= 1<<4;
//}
//
//static void clrC(){
//    cpu.AF &= ~(1<<4);
//}
