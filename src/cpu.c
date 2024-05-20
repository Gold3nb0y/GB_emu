#include "cpu.h"
#include "common.h"
#include "log.h"
#include "main_bus.h"
#include "mapper.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

static void ld_rr(byte opcode);
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg);
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg);
static void get_16bit_register_ALT(byte opcode, uint8_t offset, uint16_t** reg);

CPU_t cpu;

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
void patch(char* bytecode, size_t size){
    uint64_t i;
    for(i = 0; i < size; i++)
        cpu.bus->ROM_B0[0x100+i] = bytecode[i];
    return;
}

CPU_t* init_cpu(main_bus_t* bus){
    memset(&cpu, 0, sizeof(CPU_t));
    cpu.AF = 0x100;
    cpu.BC = 0xFF13;
    cpu.DE = 0x00C1;
    cpu.HL = 0x8403;
    cpu.SP = 0xFFFE;
    cpu.PC = 0x100;
    cpu.bus = bus;

    //give the emulator a refrence to the CPU
    return &cpu;
}

static void check_HC_add(uint8_t val1, uint8_t val2){
    uint8_t chk;
    chk = (val1 & 0xf) + (val2 & 0xf);
    if(chk & 0x10) cpu.FLAGS.HC = 1;
    else cpu.FLAGS.HC = 0;
}

static void check_HC_add_16bit(uint16_t val1, uint16_t val2){
    uint16_t chk;
    chk = (val1 & 0xfff) + (val2 & 0xfff);
    if(chk & 0x1000) cpu.FLAGS.HC = 1;
    else cpu.FLAGS.HC = 0;
}

static void check_HC_sub(uint8_t val1, uint8_t val2){
    uint8_t chk;
    chk = (val1 & 0x1F) - (val2 & 0xf);
    if(chk & 0x10) cpu.FLAGS.HC = 0; //check if the 4th bit of val1 was borrowed
    else cpu.FLAGS.HC = 1;
}

//maybe not needed?
//static void check_HC_sub_16bit(uint16_t val1, uint16_t val2){
//    uint16_t chk;
//    chk = (val1 & 0x1fff) - (val2 & 0xfff);
//    if(chk & 0x1000) cpu.FLAGS.HC = 1;
//    else cpu.FLAGS.HC = 0;
//}

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
    cpu.FLAGS.C = (result > cpu.A) ?  1 : 0;
    check_HC_sub(v1, v2);
    cpu.FLAGS.N = 1;
    return result;
}

//push does not take a refrence to anything, as there should be no need to write
static void push(uint16_t reg){
    //need to do the writing still
    cpu.SP -= 2;
    write_bus_addr(cpu.SP, reg);
}

static void pop(uint16_t* reg){
    *reg = read_bus_addr(cpu.SP);
    cpu.SP += 2;
}

static void do_ret(){
    pop(&cpu.PC); //should just be the same as popping a regular register
}

static void do_call(){
    uint16_t addr;
    addr = 0;
    addr = read_bus_addr(cpu.PC);
    cpu.PC += 2;
#ifdef DEBUG_CPU
    LOGF(DEBUG, "calling addr 0x%x\n", addr);
#endif
    push(cpu.PC);
    cpu.PC = addr;
}

static void do_jmp_immi(){
    uint16_t addr;
    addr = 0;
    addr = read_bus_addr(cpu.PC);
    cpu.PC = addr;
}

static void prefixed_instr(){
    byte opcode = read_bus(cpu.PC++);
    uint8_t *reg;
    uint8_t bit;
    uint8_t mem_val;
    uint8_t tmp_reg;

#ifdef DEBUG_CPU
    LOGF(DEBUG,"PREFIXED OPCODE: 0x%02x",opcode);
#endif
    get_8bit_register(opcode, 0, &reg);
    //potential way to handle the writeback for this addr
    if(!reg){
        mem_val = read_bus(cpu.HL);
        reg = &mem_val;
    }
    switch(opcode >> 3){
        case RLC:
            tmp_reg = *reg; 
            *reg <<= 1;
            cpu.FLAGS.C = tmp_reg >> 7;
            *reg |= cpu.FLAGS.C;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RL:
            tmp_reg = *reg; 
            *reg <<= 1;
            *reg |= cpu.FLAGS.C;
            cpu.FLAGS.C = tmp_reg >> 7;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RRC:
            tmp_reg = *reg;
            *reg >>= 1;
            *reg |= tmp_reg << 7;
            cpu.FLAGS.C = tmp_reg & 1;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RR:
            tmp_reg = *reg;
            *reg >>= 1;
            *reg |= cpu.FLAGS.C << 7;
            cpu.FLAGS.C = tmp_reg & 1;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case SLA:
            cpu.FLAGS.C = *reg >> 7;
            *reg <<= 1;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.Z = *reg ? 1 : 0;
            break;
        case SRA:
            cpu.FLAGS.C = *reg & 1;
            *reg >>= 1;
            *reg |= ((*reg >> 6) & 1) << 7; //set msb to old msb
            cpu.FLAGS.N = 0;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.Z = *reg ? 1 : 0;
            break;
        case SWAP:
            tmp_reg = *reg >> 4;
            *reg <<= 4;
            *reg |= tmp_reg;
            break;
        case SRL:
            cpu.FLAGS.C = *reg & 1;
            *reg >>= 1;
            *reg &= ~(1 << 7); //unset msb
            cpu.FLAGS.N = 0;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.Z = *reg ? 1 : 0;
            break;
        default:
            bit = (opcode >> 3) & 7;
            if(opcode >= BIT && opcode < RES){
                cpu.FLAGS.Z = *reg & 1 << bit ? 1 : 0;
                cpu.FLAGS.N = 0;
                cpu.FLAGS.HC = 1;
            } else if(opcode >= RES && opcode < SET){
                *reg &= ~(1 << bit);
            } else if (opcode >= SET) {
                *reg |= 1 << bit;
            } else {
                LOGF(ERROR, "unexpected prefixed opcode 0x%x\n", opcode);
            }
            break;
    }
    //writeback the manipulated value
    if(reg == &mem_val)
        write_bus(cpu.HL, mem_val);
}

//handle the remaining instructions that are easier to handle individually
static void basic_instr(byte opcode){
    uint16_t *temp_reg16;
    uint8_t *temp_reg8;
    uint16_t addr; 
    byte tmp_byte;
    int8_t rel_off;
    tmp_byte = addr = 0;

    switch(opcode){
        case NOP: //NOP
            break;
        case LD_BC: 
        case LD_DE: 
        case LD_HL: 
        case LD_SP: 
            get_16bit_register(opcode, 4, &temp_reg16);
            *temp_reg16 = read_bus_addr(cpu.PC);
            cpu.PC += 2;
            break;
        case STR_BC:
        case STR_DE:
        case STRI_HL:
        case STRD_HL:
            get_16bit_register(opcode, 4, &temp_reg16);
            if(opcode == STRD_HL) 
                temp_reg16 = &cpu.HL;
            write_bus(*temp_reg16, cpu.A);
            if(opcode == STRD_HL)
                cpu.HL--;
            else if(opcode == STRI_HL)
                cpu.HL++;
            break;
        case INC_BC: 
        case INC_DE: 
        case INC_HL: 
        case INC_SP: 
            get_16bit_register(opcode, 4, &temp_reg16);
            *temp_reg16 += 1;
            break;
        case ADD_HL_BC: 
        case ADD_HL_DE: 
        case ADD_HL_HL: 
        case ADD_HL_SP: //sum reg into HL
            get_16bit_register(opcode, 4, &temp_reg16);
            cpu.FLAGS.N = 0;
            check_HC_add_16bit(cpu.HL, *temp_reg16);
            cpu.HL += *temp_reg16;
            cpu.FLAGS.Z = !cpu.HL;
            break;
        case LD_A_BC:
        case LD_A_DE:
        case LDI_A_HL:
        case LDD_A_HL: //load memory value into a
            get_16bit_register(opcode, 4, &temp_reg16);
            if(opcode == LDD_A_HL) 
                temp_reg16 = &cpu.HL;
            cpu.A = read_bus(*temp_reg16);
            if(opcode == LDD_A_HL)
                cpu.HL--;
            else if(opcode == LDI_A_HL)
                cpu.HL++;
            break;
        case DEC_BC: 
        case DEC_DE: 
        case DEC_HL: 
        case DEC_SP: //dec 16 bit reg
            get_16bit_register(opcode, 4, &temp_reg16);
            *temp_reg16 -= 1;
            break;
        case INC_A:
        case INC_B:
        case INC_C:
        case INC_D:
        case INC_E:
        case INC_H:
        case INC_L:
            get_8bit_register(opcode, 3, &temp_reg8);
            cpu.FLAGS.N = 0;
            check_HC_add(*temp_reg8, 1);
            *temp_reg8 += 1;
            cpu.FLAGS.Z = !(*temp_reg8);
            break;
        case INC_MEM: //read write addr so handle seperatly
            tmp_byte = read_bus(cpu.HL);
            cpu.FLAGS.N = 0;
            check_HC_add(tmp_byte, 1);
            cpu.FLAGS.Z = !tmp_byte;
            write_bus(cpu.HL, tmp_byte++);
            break;
        case DEC_A: 
        case DEC_B: 
        case DEC_C: 
        case DEC_D: 
        case DEC_E: 
        case DEC_H: 
        case DEC_L: //dec 8bit register
            get_8bit_register(opcode, 3, &temp_reg8);
            check_HC_sub(*temp_reg8, 1);
            *temp_reg8 -= 1;
            cpu.FLAGS.Z = !(*temp_reg8);
            break;
        case DEC_MEM:
            tmp_byte = read_bus(cpu.HL);
            check_HC_sub(tmp_byte, 1);
            write_bus(cpu.HL, --tmp_byte);
            cpu.FLAGS.Z = !tmp_byte;
            break;
        case LD_A:  
        case LD_B:  
        case LD_C:  
        case LD_D:  
        case LD_E:  
        case LD_H:  
        case LD_L:  //ld 8 bit register immidiate
            get_8bit_register(opcode, 3, &temp_reg8);
            *temp_reg8 = read_bus(cpu.PC); //replace with read soon
            cpu.PC++;
            break;
        case LD_MEM:
            tmp_byte = read_bus(cpu.PC++);
            write_bus(cpu.HL, tmp_byte);
            break;
        case RLCA:
            tmp_byte = cpu.A; 
            cpu.A <<= 1;
            cpu.FLAGS.C = tmp_byte >> 7;
            cpu.A |= cpu.FLAGS.C;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RLA:
            tmp_byte = cpu.A; 
            cpu.A <<= 1;
            cpu.A |= cpu.FLAGS.C;
            cpu.FLAGS.C = cpu.A >> 7;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RRCA:
            tmp_byte = cpu.A;
            cpu.A >>= 1;
            cpu.A |= tmp_byte << 7;
            cpu.FLAGS.C = tmp_byte & 1;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case RRA:
            tmp_byte = cpu.A;
            cpu.A >>= 1;
            cpu.A |= cpu.FLAGS.C << 7;
            cpu.FLAGS.C = tmp_byte & 1;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.N = 0;
            cpu.FLAGS.Z = 0;
            break;
        case CPL:
            cpu.A = ~cpu.A;
            cpu.FLAGS.N = 1;     
            cpu.FLAGS.HC = 1;     
            break;
        case DAA:
            tmp_byte = cpu.A & 0xF; //isolate lower nibble
            //https://faculty.kfupm.edu.sa/COE/aimane/assembly/pagegen-68.aspx.htm from here
            if(tmp_byte > 10){
                tmp_byte -= 10;
                cpu.A >>= 4;
                cpu.A++;
                cpu.A <<= 4;
                cpu.A |= tmp_byte;
            }
            break;
        case CCF:
            cpu.FLAGS.N = 0;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.C = ~cpu.FLAGS.C;
            break;
        case SCF:
            cpu.FLAGS.N = 0;
            cpu.FLAGS.HC = 0;
            cpu.FLAGS.C = 1;
            break;
        case JR:
            rel_off = read_bus(cpu.PC++);
            cpu.PC += rel_off;
            break;
        case JR_NZ:
            rel_off = read_bus(cpu.PC++);
            if(!cpu.FLAGS.Z) cpu.PC += rel_off;
            break;
        case JR_Z:
            rel_off = read_bus(cpu.PC++);
            if(cpu.FLAGS.Z) cpu.PC += rel_off;
            break;
        case JR_NC:
            rel_off = read_bus(cpu.PC++);
            if(!cpu.FLAGS.C) cpu.PC += rel_off;
        case JR_C:
            rel_off = read_bus(cpu.PC++);
            if(cpu.FLAGS.C) cpu.PC += rel_off;
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
        case CB_PREFIX:
            prefixed_instr();
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
            tmp_byte = read_bus(cpu.PC);
            cpu.PC++;
            write_bus(0xFF00 + tmp_byte, cpu.A);
            break;
        case STR_DIR:
            write_bus(0xFF00 + cpu.C, cpu.A);
            break;
        case LD_DIR_n:
            tmp_byte = read_bus(cpu.PC);
            cpu.PC++;
            cpu.A = read_bus(0xFF00 + tmp_byte);
            break;
        case LD_DIR:
            cpu.A = read_bus(0xFF00 + cpu.C);
            break;
        case ADD_SP: 
            rel_off = read_bus(cpu.PC);
            cpu.PC += 1;
            check_HC_add_16bit(cpu.SP, rel_off);
            addr = cpu.SP + rel_off;
            break;
        case JMP_HL:
            addr = read_bus_addr(cpu.HL);
            cpu.PC = addr;
            break;
        case LD_MEM_A:
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
            check_HC_add_16bit(cpu.HL, cpu.SP + rel_off);
            cpu.HL = cpu.SP + rel_off;
            break;
        case LD_SP_HL:
            cpu.SP = cpu.HL;
            break;
        case STR_nn_SP:
            addr = read_bus_addr(cpu.PC);
            cpu.PC += 2;
            write_bus_addr(addr, cpu.SP);
            break;
        case STOP: //break until a button is pressed
            LOG(INFO, "Stopping execution until button is pressed, STOP INST");
            getchar();
            break;

        default:
            LOGF(ERROR, "something went wrong opcode: 0x%x\n", opcode);
            dump_cpu();
    }
}

//covered in test_arith
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
    cpu.FLAGS.Z = !result; //assign to 1 or 0 depending on weather or not the value was set
    return;
}


static void control_flow(byte opcode){
    byte op;
    uint16_t* reg; 
    get_16bit_register_ALT(opcode, 4, &reg);
    op = opcode & 0xF;
    if(op == POP){
        pop(reg);
    } else if (op == PUSH) {
        push(*reg);
    } else if ((op & 7) == RST) {
        push(cpu.PC);
        cpu.PC = ((opcode >> 3) & 7)*8;
    } else if ((op & 7) == IMMI_ARI) {
        logic_arith_8bit(op & 7, read_bus(cpu.PC));
        cpu.PC++;
    } else {
        basic_instr(opcode);
    }
    return;
}

void dump_cpu(){
    LOG(DEBUG,"---CPU contents---");
    LOGF(DEBUG,"PC: 0x%04x",cpu.PC);
    LOGF(DEBUG,"AF: 0x%04x",cpu.AF);
    LOGF(DEBUG,"BC: 0x%04x",cpu.BC);
    LOGF(DEBUG,"DE: 0x%04x",cpu.DE);
    LOGF(DEBUG,"HL: 0x%04x",cpu.HL);
    LOGF(DEBUG,"SP: 0x%04x",cpu.SP);
    LOG(DEBUG,"------------------");
}

uint64_t exec_inst(byte opcode){
    uint8_t* temp_reg;
    uint8_t tmp_val;
#ifdef DEBUG_CPU
    LOGF(DEBUG,"OPCODE: 0x%02x",opcode);
    dump_cpu();
#endif

    //seperating the instructions into groups based on the top 2 bits.
    //This allows me to use the same code to run some instructions
    switch(opcode >> 6){
        case 0: //basic instructions
            basic_instr(opcode);
            break;
        case 1: //register loads
            if(opcode == HALT) return 0; //stop the system clock
            ld_rr(opcode);
            break;
        case 2: //arithmetic and bitwise operations
            get_8bit_register(opcode, 0, &temp_reg);
            if(temp_reg == NULL){
                tmp_val = read_bus(cpu.HL);
                temp_reg = &tmp_val;
            }
            logic_arith_8bit(opcode >> 3 & 7, *temp_reg);
            break;
        case 3: //control flow, can filter some out so I figure it's worth it may change later
            control_flow(opcode);
            break;
        default:
            LOGF(ERROR,"unsupported instruction: 0x%x", opcode);
            return -1;
    }
    return 1;
}

//TODO think about running instructions in parrellel with Fetch/execute overlap
//alternatively, I can also run in sequence, and code in the different stages later.
//basically, I could try to fetch something every cycle, this would involve maintaining a transitional state 
//Maybe I could break the instructions into tasklets that preform an operation each iteration?
//another thing I coudl do is the just estimate all the registers and grab the regardless of the instruction
uint64_t exec_program(uint64_t ticks){
    byte opcode;
    uint64_t ret;

    for(uint64_t i = 0; i < ticks; i++){
        opcode = read_bus(cpu.PC++); 
        ret = exec_inst(opcode);
        if(!ret){
            return 0;
        }
    }
    return ticks;
}


//register are stored as 3 bit values in the opcode, offset if the number of bits to right shift
//this can only be used for reading with case MEM
static void get_8bit_register(byte opcode, uint8_t offset, uint8_t** reg){
    switch(opcode >> offset & 0x7){
        case B:
            *reg = &cpu.B;
            break;
        case C:
            *reg = &cpu.C;
            break;
        case D:
            *reg = &cpu.D;
            break;
        case E:
            *reg = &cpu.E;
            break;
        case H:
            *reg = &cpu.H;
            break;
        case L:
            *reg = &cpu.L;
            break;
        case MEM:
            *reg = NULL;
            break;
        case A:
            *reg = &cpu.A;
            break;
    }
}


//register are stored as 3 bit values in the opcode, offset if the number of bits to right shift
static void get_16bit_register(byte opcode, uint8_t offset, uint16_t** reg){
    switch((opcode >> offset) & 0x3){
        case BC:
            *reg = &cpu.BC;
            break;
        case DE:
            *reg = &cpu.DE;
            break;
        case HL:
            *reg = &cpu.HL;
            break;
        case SP:
            *reg = &cpu.SP;
            break;
    }
}

//there are cases where when parsing an opcode, I want to get the AF register in the 4th slot
//It is faster to make a different function to parse this
static void get_16bit_register_ALT(byte opcode, uint8_t offset, uint16_t** reg){
    switch((opcode >> offset) & 0x3){
        case BC:
            *reg = &cpu.BC;
            break;
        case DE:
            *reg = &cpu.DE;
            break;
        case HL:
            *reg = &cpu.HL;
            break;
        case SP:
            *reg = &cpu.AF;
            break;
    }
}

//register to register or memory to register load
static void ld_rr(byte opcode){
    uint8_t *dest, *src;
    uint8_t tmp;
    get_8bit_register(opcode, 3, &dest);
    get_8bit_register(opcode, 0, &src);
    if(src == NULL){
        tmp = read_bus(cpu.HL);
        src = &tmp;
    }
    if(dest == NULL){
        write_bus(cpu.HL, *src);
    } else {
        *dest = *src;
    }
}
