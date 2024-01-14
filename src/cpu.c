#include "cpu.h"
#include "common.h"
#include "main_bus.h"
#include "mapper.h"
#include <stdint.h>
#include <sys/types.h>

static void ld_rr(byte opcode);
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg);
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg);

static uint8_t tmp_reg;

//TODO I may need to implement a read byte and a read word. there are a few times where I have to read a word instead of a byte

//for arithmetic and logic
#define ADD 0
#define ADC 1
#define SUB 2
#define SBC 3
#define AND 4
#define XOR 5
#define OR  6
#define CP  7

//for the upper ones
#define POP 1
#define PUSH 5
#define IMMI_ARI 6
#define RST 6

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

static void check_HC_add(uint8_t val1, uint8_t val2){
    uint8_t chk;
    chk = val1 & 0xf + val2 & 0xf;
    if(chk & 0x10) cpu.FLAGS.HC = 1;
    else cpu.FLAGS.HC = 0;
}

static void check_HC_sub(uint8_t val1, uint8_t val2){
    uint8_t chk;
    chk = val1 & 0x10 - val2 & 0xf;
    if(chk & 0x10) cpu.FLAGS.HC = 0; //check if the 5th bit of val1 was borrowed
    else cpu.FLAGS.HC = 1;
}

static uint8_t add(byte v1, byte v2){
    uint8_t result;
    result = v1 + v2;
    if(result < cpu.A) cpu.FLAGS.C = 1;
    else cpu.FLAGS.C = 0;
    check_HC_add(v1, v2);
    cpu.FLAGS.N = 0;
    return result;
}

static uint8_t sub(byte v1, byte v2){
    uint8_t result;
    result = v1 - v2;
    cpu.FLAGS.C = (result < cpu.A) ?  1 : 0;
    check_HC_sub(v1, v2);
    cpu.FLAGS.N = 1;
    return result;
}

//push does not take a refrence to anything, as there should be no need to write
static void push(uint16_t reg){
    //need to do the writing still
    write_bus_addr(cpu.PC, reg);
    cpu.SP -= 2;
}

static void pop(uint16_t* reg){
    *reg = read_bus_addr(cpu.SP);
    cpu.SP += 2;
}

static void do_ret(){
    //cpu.PC = read_bus_addr(cpu.SP);
    pop(&cpu.PC); //should just be the same as popping a regular register
}

static void do_call(){
    uint16_t addr;
    addr = 0;
    addr = read_bus_addr(cpu.PC);
    cpu.PC += 2;
    push(cpu.PC);
    cpu.PC = addr;
}

static void do_jmp_immi(){
    uint16_t addr;
    addr = 0;
    addr = read_bus_addr(cpu.PC);
    cpu.PC = addr;
}

//handle the remaining instructions that don't really follow a set pattern
static void handle_misc(byte opcode){
    uint16_t addr; 
    addr = 0;
    int8_t rel_off;
    byte n;

    switch(opcode){
        case RET_NZ:
            if(!cpu.FLAGS.Z) do_ret();
            break;
        case JNZ:
            if(!cpu.FLAGS.Z) do_jmp_immi();
            break;
        case JMP:
            if(!cpu.FLAGS.Z) do_jmp_immi();
            break;
        case CALL_NZ:
            if(!cpu.FLAGS.Z) do_call();
            break;
        case RET_Z:
            if(cpu.FLAGS.Z) do_ret();
            break;
        case RET:
            do_ret();
            break;
        case JZ:
            if(cpu.FLAGS.Z) do_jmp_immi();
            break;
        case CB_op:
            LOG(INFO, "trigger special instruction set");
            break;
        case CALL_Z:
            if(cpu.FLAGS.Z) do_call();
            break;
        case CALL:
            do_call();
            break;
        case RET_NC:
            if(!cpu.FLAGS.C) do_ret();
            break;
        case CALL_NC:
            if(!cpu.FLAGS.C) do_call();
            break;
        case RET_C:
            if(cpu.FLAGS.C) do_call();
            break;
        case RETI:
            do_ret();
            cpu.IME = 1;
            break;
        case CALL_C:
            if(cpu.FLAGS.C) do_call();
            break;
        case STR_DIR_n:
            n = read_bus(cpu.PC);
            cpu.PC++;
            write_bus(0xFF00 + n, cpu.A);
            break;
        case STR_DIR:
            write_bus(0xFF00 + cpu.C, cpu.A);
            break;
        case LD_DIR_n:
            n = read_bus(cpu.PC);
            cpu.PC++;
            write_bus(0xFF00 + n, cpu.A);
            break;
        case LD_DIR:
            write_bus(0xFF00 + cpu.C, cpu.A);
            break;
        case ADD_SP: 
            rel_off = read_bus(cpu.PC);
            cpu.PC += 1;
            //be careful that the signed property is being transfered
            addr = cpu.SP + rel_off;
            //not sure how to check the half carry
            //check_HC();
            //check carry
        case JMP_HL:
            addr = read_bus_addr(cpu.HL);
            cpu.PC = addr;
            break;
        case LD_MEM_A:
            LOG(ERROR, "NOT YET IMPLEMENTED");
            addr = read_bus_addr(cpu.PC);
            cpu.PC += 2;            
            write_bus(addr, cpu.A);
            break;
        case LD_A_MEM:
            addr = read_bus_addr(cpu.PC);
            cpu.PC += 2;            
            cpu.A = read_bus(addr);
            break;
        case DI:
            cpu.IME = 0;
            break;
        case EI:
            cpu.IME = 1;
            break;
        case LD_HL_SP_e:
            rel_off = read_bus(cpu.PC);
            cpu.PC++;
            cpu.HL = cpu.SP + rel_off;
            //check_HC();
            //check carry
            break;
        case LD_SP_HL:
            cpu.SP = cpu.HL;
            break;

        default:
            LOG(ERROR, "something went wrong");
    }
}


void logic_arith_8bit(byte operation, uint8_t value){
    uint8_t result;

    switch(operation){ 
        case ADD:
            result = add(cpu.A, value);
            cpu.A = result;
            break;
        case ADC:
            value += cpu.FLAGS.C;
            result = add(cpu.A, value);
            cpu.A = result;
            break;
        case SUB:
            result = sub(cpu.A, value);
            cpu.A = result;
            break;
        case SBC:
            value -= cpu.FLAGS.C;
            result = sub(cpu.A, value);
            cpu.A = result;
            break;
        case AND:
            result = cpu.A & value;
            cpu.FLAGS.HC = 1;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.C = 0;
            cpu.A = result;
            break;
        case XOR:
            result = cpu.A ^ value;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.C = 0;
            cpu.A = result;
            break;
        case OR:
            result = cpu.A | value;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.C = 0;
            cpu.A = result;
            break;
        case CP: //same as sub but does not update A
            result = sub(cpu.A, value);
            break;
    }
    //ZF can be set at the end since the arithmetic is the same
    if(result) cpu.FLAGS.Z = 0;
    else cpu.FLAGS.Z = 1;
    return;
}


static void control_flow(byte opcode){
    byte op;
    uint16_t* reg; 
    get_16bit_register(opcode, 4, &reg);
    op = opcode & 0xF;
    if(op == POP){
        pop(reg);
    } else if (op == PUSH) {
        push(*reg);
    } else if ((op & 7) == RST) {
        LOG(ERROR, "Not implemented");
    } else if ((op & 7) == IMMI_ARI) {
        logic_arith_8bit(op & 7, cpu.bus->ROM_B0[cpu.PC++]);
    } else {
        handle_misc(opcode);
    }
    return;
}

//TODO think about running instructions in parrellel with Fetch/execute overlap
//alternatively, I can also run in sequence, and code in the different stages later.
//basically, I could try to fetch something every cycle, this would involve maintaining a transitional state 
//Maybe I could break the instructions into tasklets that preform an operation each iteration?
//another thing I coudl do is the just estimate all the registers and grab the regardless of the instruction
uint64_t exec(uint64_t ticks){
    byte opcode;
    uint8_t* temp_reg;

    for(uint64_t i = 0; i < ticks; i++){
        opcode = cpu.bus->ROM_B0[cpu.PC];
        cpu.PC++;
#ifdef DEBUG_CPU
        LOG(DEBUG,"---CPU contents---");
        LOGF(DEBUG,"OPCODE: 0x%02x",opcode);
        LOGF(DEBUG,"AF: 0x%04x",cpu.AF);
        LOGF(DEBUG,"BC: 0x%04x",cpu.BC);
        LOGF(DEBUG,"DE: 0x%04x",cpu.DE);
        LOGF(DEBUG,"HL: 0x%04x",cpu.HL);
        LOGF(DEBUG,"PC: 0x%04x",cpu.PC);
        LOGF(DEBUG,"PC: 0x%04x",cpu.SP);
        LOG(DEBUG,"------------------");
#endif

        //seperating the instructions into groups based on the top 2 bits.
        //This allows me to use the same code to run some instructions
        switch(opcode >> 6){
            case 0:
                single_reg_inst(opcode);
                break;
            case 1: //register loads
                if(opcode == HALT) return 0; //stop the system clock
                ld_rr(opcode);
                break;
            case 2: //arithmetic and bitwise operations
                get_8bit_register(opcode, 0, &temp_reg);
                logic_arith_8bit(opcode >> 3 & 7, *temp_reg);
                break;
            case 3: //control flow
                control_flow(opcode);
                break;
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
        case 6:
            tmp_reg = read_bus(cpu.HL);
            *reg = &tmp_reg;
        case 7:
            *reg = &cpu.A;
            break;
        default:
            LOG(ERROR, "something unexpected happened");
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
