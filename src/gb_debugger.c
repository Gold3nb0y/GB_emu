#include <gb_debugger.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

debugger_t db;
const instr instr_table[];

char* read_command(){
    char c;
    uint64_t cur, lim, bytes_read;
    char* buf;

    printf("dbg > ");

    buf = malloc(0x10);
    lim = 0x10;
    cur = c = 0;
    do{
        bytes_read = read(STDIN_FILENO,&c,1);
        buf[cur++] = c; 
        if(cur >= lim){
            lim *= 2;
            buf = realloc(buf, lim);
        }
    } while(c != '\n' && c != 0 && bytes_read == 1);

    return buf;
}

void break_point(char *cmd){
    address addr;
    if(db.num_bp >= db.max_bp){
        printf("no availiable breakpoints");
        return;
    }
    sscanf(cmd, "b %hx", &addr);
    db.breakpoints[db.num_bp++] = addr;
    printf("Breakpoint created at 0x%04x\n", addr);
}

void delete_break(char *cmd){
    uint idx;
    idx = atoi(cmd+2);
    if(idx >= db.max_bp){
        printf("invalid index");
        return;
    }
    db.breakpoints[idx] = 0;
}

void examine(char *cmd){
    address addr;
    byte b;
    byte line[0x10];
    uint32_t count;

    sscanf(cmd, "x %hx %d", &addr, &count);

    for(uint32_t i = 0; i < count; i++){
        if(i % 0x10 == 0)
            printf("0x%04x : ", addr + i);
        b = read_bus(addr + i);
        line[i%10] = b;
        printf("0x%02x ", b);
        if(i % 0x10 == 0xF)
            printf(" |%s|\n", line);
    }
    puts("");
    return;
}

void step(){
    exec_program(1);
    for(int i = 0; i < 8; i++)
        ppu_tick(); //tick ppu 8 times per cpu cycle
}

void run_until_break(){
    uint64_t i;
    for(;;){
        step();
        //a bit of overhead, but nothing too bad not worrying about DMA right now
        for(i = 0; i < db.num_bp; i++){

            if(db.cpu->PC == db.breakpoints[i]) return;
        }
    }
}

void disassemble(address addr, uint64_t count){
    uint16_t step;
    uint64_t i;
    byte b;
    address a;
    instr inst;
    byte opcode;
    step = i = 0;
    for(; i < count; i++){
        opcode = read_bus(addr + step);
        memcpy(&inst, &instr_table[opcode], sizeof(instr));
        printf("0x%04x: ", addr + step);
        step++;
        if(inst.size == 2){
            b = read_bus(addr + step++);
            printf(inst.instr_fmt, b);
            puts("");
        } else if(inst.size == 3){
            a = read_bus_addr(addr + step);
            printf(inst.instr_fmt, a);
            puts("");
            step += 2;
        } else {
            puts(inst.instr_fmt);
        }
    }
    return;
}

#define SET_PRE "\033[92;1;4m"
#define SUF "\033[m"

void status(){
    puts("-------------CPU contents-------------");
    printf("PC: 0x%04x\n",db.cpu->PC);
    printf("AF: 0x%04x A: 0x%02x\n",db.cpu->AF, db.cpu->A);
    printf("BC: 0x%04x B: 0x%02x C: 0x%02x\n",db.cpu->BC, db.cpu->B, db.cpu->C);
    printf("DE: 0x%04x D: 0x%02x E: 0x%02x\n",db.cpu->DE, db.cpu->D, db.cpu->E);
    printf("HL: 0x%04x H: 0x%02x L: 0x%02x\n",db.cpu->HL, db.cpu->H, db.cpu->L);
    printf("SP: 0x%04x\n",db.cpu->SP);
    printf("FLAGS: ");
    if(db.cpu->FLAGS.Z) printf(SET_PRE);
    printf("ZERO");
    if(db.cpu->FLAGS.Z) printf(SUF);
    printf(" ");
    if(db.cpu->FLAGS.N) printf(SET_PRE);
    printf("NEG");
    if(db.cpu->FLAGS.N) printf(SUF);
    printf(" ");
    if(db.cpu->FLAGS.C) printf(SET_PRE);
    printf("CARRY");
    if(db.cpu->FLAGS.C) printf(SUF);
    printf(" ");
    if(db.cpu->FLAGS.HC) printf(SET_PRE);
    printf("HALF");
    if(db.cpu->FLAGS.HC) printf(SUF);
    puts("");
    puts("-------------DISASSEMBLY--------------");
    disassemble(db.cpu->PC, 10);
    puts("---------------STACK------------------");
    for(uint i = 0; i < 10; i += 2){
        printf("0x%04x: 0x%04x\n", db.cpu->SP + i, read_bus_addr(db.cpu->SP + i));
    }
    puts("--------------------------------------");
}

//like the gdb implementation of ni
void next(){
    address addr = 0;
    byte cur_op;
    cur_op = read_bus(db.cpu->PC);
    if(strncmp(instr_table[cur_op].instr_fmt, "CALL", 4) == 0){
        addr = db.cpu->PC + instr_table[cur_op].size; //address of next instr
        do{
            step();
        }while(db.cpu->PC != addr);
    } else {
        step();
    }
    return;
}

void debug(){
    char* cmd;
    bool done = false;
    uint64_t count = 0;
    address addr;

    status();
    while(!done){
        cmd = read_command();
        switch(cmd[0]){
            case 'a': 
                sscanf(cmd, "a %hx %ld", &addr, &count);
                if(count < 0x100)
                    disassemble(addr, count);
                break;
            case 'b': 
                break_point(cmd);
                break;
            case 'c':
                run_until_break();
                status();
                break;
            case 'd':
                delete_break(cmd);
                break;
            case 'i':
                status();
                break;
            case 'n':
                next();
                status();
                break;
            case 's':
                step();
                status();
                break;
            case 'q':
                done = true;
                break;
            case 'x':
                examine(cmd);
                break;
            default:
                puts("Invalid command");
        }
        free(cmd);
    }
    exit(0);
}

void start_debugger(main_bus_t *bus, CPU_t *cpu){
    db.bus = bus;
    db.cpu = cpu;
    db.breakpoints = calloc(0x10, sizeof(address));
    db.num_bp = 0;
    db.max_bp = 0x10;
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    signal(SIGINT, debug);
    debug();
}

#ifndef NATTACH_DB
const instr instr_table[] = {
    [NOP]           = {.opcode = NOP,   .instr_fmt = "NOP",      .size = 1},
    [LD_BC]         = {.opcode = LD_BC,     .instr_fmt = "LD BC, 0x%04x",  .size = 3},
    [STR_BC]        = {.opcode = STR_BC,    .instr_fmt = "STR (BC), A",      .size = 1},
    [INC_BC]        = {.opcode = INC_BC,    .instr_fmt = "INC BC",      .size = 1},
    [INC_B]         = {.opcode = INC_B,     .instr_fmt = "INC B",      .size = 1},
    [DEC_B]         = {.opcode = DEC_B,     .instr_fmt = "DEC B",      .size = 1},
    [LD_B]          = {.opcode = LD_B,  .instr_fmt = "LD B, 0x%02x",       .size = 2},
    [RLCA]          = {.opcode = RLCA,  .instr_fmt = "RLCA",         .size = 1},
    [STR_nn_SP]         = {.opcode = STR_nn_SP,     .instr_fmt = "STR 0x%04x, SP",      .size = 3},
    [ADD_HL_BC]         = {.opcode = ADD_HL_BC,     .instr_fmt = "ADD HL, BC",      .size = 1},
    [LD_A_BC]       = {.opcode = LD_A_BC,   .instr_fmt = "LD A, BC",       .size = 1},
    [DEC_BC]        = {.opcode = DEC_BC,    .instr_fmt = "DEC BC",      .size = 1},
    [INC_C]         = {.opcode = INC_C,     .instr_fmt = "INC C",      .size = 1},
    [DEC_C]         = {.opcode = DEC_C,     .instr_fmt = "DEC C",      .size = 1},
    [LD_C]          = {.opcode = LD_C,  .instr_fmt = "LD C, 0x%02x",       .size = 2},
    [RRCA]          = {.opcode = RRCA,  .instr_fmt = "RRCA",         .size = 1},
    [STOP]          = {.opcode = STOP,  .instr_fmt = "STOP",         .size = 2},
    [LD_DE]         = {.opcode = LD_DE,     .instr_fmt = "LD DE, 0x%04x",       .size = 3},
    [STR_DE]        = {.opcode = STR_DE,    .instr_fmt = "STR DE",      .size = 1},
    [INC_DE]        = {.opcode = INC_DE,    .instr_fmt = "INC DE",      .size = 1},
    [INC_D]         = {.opcode = INC_D,     .instr_fmt = "INC D",      .size = 1},
    [DEC_D]         = {.opcode = DEC_D,     .instr_fmt = "DEC D",      .size = 1},
    [LD_D]          = {.opcode = LD_D,  .instr_fmt = "LD D, 0x%02x",       .size = 2},
    [RLA]           = {.opcode = RLA,   .instr_fmt = "RLA",      .size = 1},
    [JR]            = {.opcode = JR,    .instr_fmt = "JR %hhd",       .size = 2},
    [ADD_HL_DE]         = {.opcode = ADD_HL_DE,     .instr_fmt = "ADD HL, DE",      .size = 1},
    [LD_A_DE]       = {.opcode = LD_A_DE,   .instr_fmt = "LD A, (DE)",       .size = 1},
    [DEC_DE]        = {.opcode = DEC_DE,    .instr_fmt = "DEC DE",      .size = 1},
    [INC_E]         = {.opcode = INC_E,     .instr_fmt = "INC E",      .size = 1},
    [DEC_E]         = {.opcode = DEC_E,     .instr_fmt = "DEC E",      .size = 1},
    [LD_E]          = {.opcode = LD_E,  .instr_fmt = "LD E, 0x%02x",       .size = 2},
    [RRA]           = {.opcode = RRA,   .instr_fmt = "RRA",      .size = 1},
    [JR_NZ]         = {.opcode = JR_NZ,     .instr_fmt = "JRNZ %hhd",       .size = 2},
    [LD_HL]         = {.opcode = LD_HL,     .instr_fmt = "LD HL, 0x%04x",       .size = 3},
    [STRI_HL]       = {.opcode = STRI_HL,   .instr_fmt = "STR (HL++), A",         .size = 1},
    [INC_HL]        = {.opcode = INC_HL,    .instr_fmt = "INC HL",      .size = 1},
    [INC_H]         = {.opcode = INC_H,     .instr_fmt = "INC H",      .size = 1},
    [DEC_H]         = {.opcode = DEC_H,     .instr_fmt = "DEC H",      .size = 1},
    [LD_H]          = {.opcode = LD_H,  .instr_fmt = "LD H, 0x%02x",         .size = 2},
    [DAA]           = {.opcode = DAA,   .instr_fmt = "DAA",      .size = 1},
    [JR_Z]          = {.opcode = JR_Z,  .instr_fmt = "JRZ %hhd",       .size = 2},
    [ADD_HL_HL]         = {.opcode = ADD_HL_HL,     .instr_fmt = "ADD HL, HL",      .size = 1},
    [LDI_A_HL]      = {.opcode = LDI_A_HL,  .instr_fmt = "LDI A, (HL++)",      .size = 1},
    [DEC_HL]        = {.opcode = DEC_HL,    .instr_fmt = "DEC HL",      .size = 1},
    [INC_L]         = {.opcode = INC_L,     .instr_fmt = "INC L",      .size = 1},
    [DEC_L]         = {.opcode = DEC_L,     .instr_fmt = "DEC L",      .size = 1},
    [LD_L]          = {.opcode = LD_L,  .instr_fmt = "LD L, 0x%02x",       .size = 2},
    [CPL]           = {.opcode = CPL,   .instr_fmt = "CPL",      .size = 1},
    [JR_NC]         = {.opcode = JR_NC,     .instr_fmt = "JRNC %hhd",       .size = 2},
    [LD_SP]         = {.opcode = LD_SP,     .instr_fmt = "LD SP, 0x%04x",       .size = 3},
    [STRD_HL]       = {.opcode = STRD_HL,   .instr_fmt = "STRD (HL--), A",         .size = 1},
    [INC_SP]        = {.opcode = INC_SP,    .instr_fmt = "INC SP",      .size = 1},
    [INC_MEM]       = {.opcode = INC_MEM,   .instr_fmt = "INC (HL)",      .size = 1},
    [DEC_MEM]       = {.opcode = DEC_MEM,   .instr_fmt = "DEC (HL)",      .size = 1},
    [LD_MEM]        = {.opcode = LD_MEM,    .instr_fmt = "LD (HL), 0x%02x",       .size = 2},
    [SCF]           = {.opcode = SCF,   .instr_fmt = "SCF",      .size = 1},
    [JR_C]          = {.opcode = JR_C,  .instr_fmt = "JRC %hhd",       .size = 2},
    [ADD_HL_SP]         = {.opcode = ADD_HL_SP,     .instr_fmt = "ADD HL, SP",      .size = 1},
    [LDD_A_HL]      = {.opcode = LDD_A_HL,  .instr_fmt = "LD A, (HL--)",      .size = 1},
    [DEC_SP]        = {.opcode = DEC_SP,    .instr_fmt = "DEC SP",      .size = 1},
    [INC_A]         = {.opcode = INC_A,     .instr_fmt = "INC A",     .size = 1},
    [DEC_A]         = {.opcode = DEC_A,     .instr_fmt = "DEC A",      .size = 1},
    [LD_A]          = {.opcode = LD_A,  .instr_fmt = "LD A, 0x%02x",       .size = 2},
    [CCF]           = {.opcode = CCF,   .instr_fmt = "CCF",      .size = 1},
    [LD_B_B]        = {.opcode = LD_B_B,    .instr_fmt = "LD B, B",       .size = 1},
    [LD_B_C]        = {.opcode = LD_B_C,    .instr_fmt = "LD B, C",       .size = 1},
    [LD_B_D]        = {.opcode = LD_B_D,    .instr_fmt = "LD B, D",       .size = 1},
    [LD_B_E]        = {.opcode = LD_B_E,    .instr_fmt = "LD B, E",       .size = 1},
    [LD_B_H]        = {.opcode = LD_B_H,    .instr_fmt = "LD B, H",       .size = 1},
    [LD_B_L]        = {.opcode = LD_B_L,    .instr_fmt = "LD B, L",       .size = 1},
    [LD_B_MEM_HL]       = {.opcode = LD_B_MEM_HL,   .instr_fmt = "LD B, (HL)",       .size = 1},
    [LD_B_A]        = {.opcode = LD_B_A,    .instr_fmt = "LD B, A",       .size = 1},
    [LD_C_B]        = {.opcode = LD_C_B,    .instr_fmt = "LD C, B",       .size = 1},
    [LD_C_C]        = {.opcode = LD_C_C,    .instr_fmt = "LD C, C",       .size = 1},
    [LD_C_D]        = {.opcode = LD_C_D,    .instr_fmt = "LD C, D",       .size = 1},
    [LD_C_E]        = {.opcode = LD_C_E,    .instr_fmt = "LD C, E",       .size = 1},
    [LD_C_H]        = {.opcode = LD_C_H,    .instr_fmt = "LD C, H",       .size = 1},
    [LD_C_L]        = {.opcode = LD_C_L,    .instr_fmt = "LD C, L",       .size = 1},
    [LD_C_MEM_HL]       = {.opcode = LD_C_MEM_HL,   .instr_fmt = "LD C, (HL)",       .size = 1},
    [LD_C_A]        = {.opcode = LD_C_A,    .instr_fmt = "LD C, A",       .size = 1},
    [LD_D_B]        = {.opcode = LD_D_B,    .instr_fmt = "LD D, B",       .size = 1},
    [LD_D_C]        = {.opcode = LD_D_C,    .instr_fmt = "LD D, C",       .size = 1},
    [LD_D_D]        = {.opcode = LD_D_D,    .instr_fmt = "LD D, D",       .size = 1},
    [LD_D_E]        = {.opcode = LD_D_E,    .instr_fmt = "LD D, E",       .size = 1},
    [LD_D_H]        = {.opcode = LD_D_H,    .instr_fmt = "LD D, H",       .size = 1},
    [LD_D_L]        = {.opcode = LD_D_L,    .instr_fmt = "LD D, L",       .size = 1},
    [LD_D_MEM_HL]       = {.opcode = LD_D_MEM_HL,   .instr_fmt = "LD D, (HL)",       .size = 1},
    [LD_D_A]        = {.opcode = LD_D_A,    .instr_fmt = "LD D, A",       .size = 1},
    [LD_E_B]        = {.opcode = LD_E_B,    .instr_fmt = "LD E, B",       .size = 1},
    [LD_E_C]        = {.opcode = LD_E_C,    .instr_fmt = "LD E, C",       .size = 1},
    [LD_E_D]        = {.opcode = LD_E_D,    .instr_fmt = "LD E, D",       .size = 1},
    [LD_E_E]        = {.opcode = LD_E_E,    .instr_fmt = "LD E, E",       .size = 1},
    [LD_E_H]        = {.opcode = LD_E_H,    .instr_fmt = "LD E, H",       .size = 1},
    [LD_E_L]        = {.opcode = LD_E_L,    .instr_fmt = "LD E, L",       .size = 1},
    [LD_E_MEM_HL]       = {.opcode = LD_E_MEM_HL,   .instr_fmt = "LD E, (HL)",       .size = 1},
    [LD_E_A]        = {.opcode = LD_E_A,    .instr_fmt = "LD E, A",       .size = 1},
    [LD_H_B]        = {.opcode = LD_H_B,    .instr_fmt = "LD H, B",       .size = 1},
    [LD_H_C]        = {.opcode = LD_H_C,    .instr_fmt = "LD H, C",       .size = 1},
    [LD_H_D]        = {.opcode = LD_H_D,    .instr_fmt = "LD H, D",       .size = 1},
    [LD_H_E]        = {.opcode = LD_H_E,    .instr_fmt = "LD H, E",       .size = 1},
    [LD_H_H]        = {.opcode = LD_H_H,    .instr_fmt = "LD H, H",       .size = 1},
    [LD_H_L]        = {.opcode = LD_H_L,    .instr_fmt = "LD H, L",       .size = 1},
    [LD_H_MEM_HL]       = {.opcode = LD_H_MEM_HL,   .instr_fmt = "LD H, (HL)",       .size = 1},
    [LD_H_A]        = {.opcode = LD_H_A,    .instr_fmt = "LD H, A",       .size = 1},
    [LD_L_B]        = {.opcode = LD_L_B,    .instr_fmt = "LD L, B",       .size = 1},
    [LD_L_C]        = {.opcode = LD_L_C,    .instr_fmt = "LD L, C",       .size = 1},
    [LD_L_D]        = {.opcode = LD_L_D,    .instr_fmt = "LD L, D",       .size = 1},
    [LD_L_E]        = {.opcode = LD_L_E,    .instr_fmt = "LD L, E",       .size = 1},
    [LD_L_H]        = {.opcode = LD_L_H,    .instr_fmt = "LD L, H",       .size = 1},
    [LD_L_L]        = {.opcode = LD_L_L,    .instr_fmt = "LD L, L",       .size = 1},
    [LD_L_MEM_HL]       = {.opcode = LD_L_MEM_HL,   .instr_fmt = "LD L, (HL)",       .size = 1},
    [LD_L_A]        = {.opcode = LD_L_A,    .instr_fmt = "LD L, A",       .size = 1},
    [LD_MEM_HL_B]       = {.opcode = LD_MEM_HL_B,   .instr_fmt = "LD (HL), B",       .size = 1},
    [LD_MEM_HL_C]       = {.opcode = LD_MEM_HL_C,   .instr_fmt = "LD (HL), C",       .size = 1},
    [LD_MEM_HL_D]       = {.opcode = LD_MEM_HL_D,   .instr_fmt = "LD (HL), D",       .size = 1},
    [LD_MEM_HL_E]       = {.opcode = LD_MEM_HL_E,   .instr_fmt = "LD (HL), E",       .size = 1},
    [LD_MEM_HL_H]       = {.opcode = LD_MEM_HL_H,   .instr_fmt = "LD (HL), H",       .size = 1},
    [LD_MEM_HL_L]       = {.opcode = LD_MEM_HL_L,   .instr_fmt = "LD (HL), L",       .size = 1},
    [HALT]          = {.opcode = HALT,  .instr_fmt = "HALT",         .size = 1},
    [LD_MEM_HL_A]       = {.opcode = LD_MEM_HL_A,   .instr_fmt = "LD (HL), A",       .size = 1},
    [LD_A_B]        = {.opcode = LD_A_B,    .instr_fmt = "LD A, B",       .size = 1},
    [LD_A_C]        = {.opcode = LD_A_C,    .instr_fmt = "LD A, C",       .size = 1},
    [LD_A_D]        = {.opcode = LD_A_D,    .instr_fmt = "LD A, D",       .size = 1},
    [LD_A_E]        = {.opcode = LD_A_E,    .instr_fmt = "LD A, E",       .size = 1},
    [LD_A_H]        = {.opcode = LD_A_H,    .instr_fmt = "LD A, H",       .size = 1},
    [LD_A_L]        = {.opcode = LD_A_L,    .instr_fmt = "LD A, L",       .size = 1},
    [LD_A_MEM_HL]       = {.opcode = LD_A_MEM_HL,   .instr_fmt = "LD A, (HL)",       .size = 1},
    [LD_A_A]        = {.opcode = LD_A_A,    .instr_fmt = "LD A, A",       .size = 1},
    [ADD_B]         = {.opcode = ADD_B,     .instr_fmt = "ADD A, B",      .size = 1},
    [ADD_C]         = {.opcode = ADD_C,     .instr_fmt = "ADD A, C",      .size = 1},
    [ADD_D]         = {.opcode = ADD_D,     .instr_fmt = "ADD A, D",      .size = 1},
    [ADD_E]         = {.opcode = ADD_E,     .instr_fmt = "ADD A, E",      .size = 1},
    [ADD_H]         = {.opcode = ADD_H,     .instr_fmt = "ADD A, H",      .size = 1},
    [ADD_L]         = {.opcode = ADD_L,     .instr_fmt = "ADD A, L",      .size = 1},
    [ADD_MEM_HL]        = {.opcode = ADD_MEM_HL,    .instr_fmt = "ADD A, (HL)",      .size = 1},
    [ADD_A]         = {.opcode = ADD_A,     .instr_fmt = "ADD A, A",      .size = 1},
    [ADC_B]         = {.opcode = ADC_B,     .instr_fmt = "ADC A, B",      .size = 1},
    [ADC_C]         = {.opcode = ADC_C,     .instr_fmt = "ADC A, C",      .size = 1},
    [ADC_D]         = {.opcode = ADC_D,     .instr_fmt = "ADC A, D",      .size = 1},
    [ADC_E]         = {.opcode = ADC_E,     .instr_fmt = "ADC A, E",      .size = 1},
    [ADC_H]         = {.opcode = ADC_H,     .instr_fmt = "ADC A, H",      .size = 1},
    [ADC_L]         = {.opcode = ADC_L,     .instr_fmt = "ADC A, L",      .size = 1},
    [ADC_MEM_HL]        = {.opcode = ADC_MEM_HL,    .instr_fmt = "ADC A, (HL)",      .size = 1},
    [ADC_A]         = {.opcode = ADC_A,     .instr_fmt = "ADC A, A",      .size = 1},
    [SUB_B]         = {.opcode = SUB_B,     .instr_fmt = "SUB A, B",      .size = 1},
    [SUB_C]         = {.opcode = SUB_C,     .instr_fmt = "SUB A, C",      .size = 1},
    [SUB_D]         = {.opcode = SUB_D,     .instr_fmt = "SUB A, D",      .size = 1},
    [SUB_E]         = {.opcode = SUB_E,     .instr_fmt = "SUB A, E",      .size = 1},
    [SUB_H]         = {.opcode = SUB_H,     .instr_fmt = "SUB A, H",      .size = 1},
    [SUB_L]         = {.opcode = SUB_L,     .instr_fmt = "SUB A, L",      .size = 1},
    [SUB_MEM_HL]        = {.opcode = SUB_MEM_HL,    .instr_fmt = "SUB A, (HL)",      .size = 1},
    [SUB_A]         = {.opcode = SUB_A,     .instr_fmt = "SUB A, A",      .size = 1},
    [SBC_B]         = {.opcode = SBC_B,     .instr_fmt = "SBC A, B",      .size = 1},
    [SBC_C]         = {.opcode = SBC_C,     .instr_fmt = "SBC A, C",      .size = 1},
    [SBC_D]         = {.opcode = SBC_D,     .instr_fmt = "SBC A, D",      .size = 1},
    [SBC_E]         = {.opcode = SBC_E,     .instr_fmt = "SBC A, E",      .size = 1},
    [SBC_H]         = {.opcode = SBC_H,     .instr_fmt = "SBC A, H",      .size = 1},
    [SBC_L]         = {.opcode = SBC_L,     .instr_fmt = "SBC A, L",      .size = 1},
    [SBC_MEM_HL]        = {.opcode = SBC_MEM_HL,    .instr_fmt = "SBC A, (HL)",      .size = 1},
    [SBC_A]         = {.opcode = SBC_A,     .instr_fmt = "SBC A, A",      .size = 1},
    [AND_B]         = {.opcode = AND_B,     .instr_fmt = "AND A, B",      .size = 1},
    [AND_C]         = {.opcode = AND_C,     .instr_fmt = "AND A, C",      .size = 1},
    [AND_D]         = {.opcode = AND_D,     .instr_fmt = "AND A, D",      .size = 1},
    [AND_E]         = {.opcode = AND_E,     .instr_fmt = "AND A, E",      .size = 1},
    [AND_H]         = {.opcode = AND_H,     .instr_fmt = "AND A, H",      .size = 1},
    [AND_L]         = {.opcode = AND_L,     .instr_fmt = "AND A, L",      .size = 1},
    [AND_MEM_HL]        = {.opcode = AND_MEM_HL,    .instr_fmt = "AND A, (HL)",      .size = 1},
    [AND_A]         = {.opcode = AND_A,     .instr_fmt = "AND A, A",      .size = 1},
    [XOR_B]         = {.opcode = XOR_B,     .instr_fmt = "XOR A, B",      .size = 1},
    [XOR_C]         = {.opcode = XOR_C,     .instr_fmt = "XOR A, C",      .size = 1},
    [XOR_D]         = {.opcode = XOR_D,     .instr_fmt = "XOR A, D",      .size = 1},
    [XOR_E]         = {.opcode = XOR_E,     .instr_fmt = "XOR A, E",      .size = 1},
    [XOR_H]         = {.opcode = XOR_H,     .instr_fmt = "XOR A, H",      .size = 1},
    [XOR_L]         = {.opcode = XOR_L,     .instr_fmt = "XOR A, L",      .size = 1},
    [XOR_MEM_HL]        = {.opcode = XOR_MEM_HL,    .instr_fmt = "XOR A, (HL)",      .size = 1},
    [XOR_A]         = {.opcode = XOR_A,     .instr_fmt = "XOR A, A",      .size = 1},
    [OR_B]          = {.opcode = OR_B,  .instr_fmt = "OR A, B",       .size = 1},
    [OR_C]          = {.opcode = OR_C,  .instr_fmt = "OR A, C",       .size = 1},
    [OR_D]          = {.opcode = OR_D,  .instr_fmt = "OR A, D",       .size = 1},
    [OR_E]          = {.opcode = OR_E,  .instr_fmt = "OR A, E",       .size = 1},
    [OR_H]          = {.opcode = OR_H,  .instr_fmt = "OR A, H",       .size = 1},
    [OR_L]          = {.opcode = OR_L,  .instr_fmt = "OR A, L",       .size = 1},
    [OR_MEM_HL]         = {.opcode = OR_MEM_HL,     .instr_fmt = "OR A, (HL)",       .size = 1},
    [OR_A]          = {.opcode = OR_A,  .instr_fmt = "OR A, A",       .size = 1},
    [CP_B]          = {.opcode = CP_B,  .instr_fmt = "CP A, B",       .size = 1},
    [CP_C]          = {.opcode = CP_C,  .instr_fmt = "CP A, C",       .size = 1},
    [CP_D]          = {.opcode = CP_D,  .instr_fmt = "CP A, D",       .size = 1},
    [CP_E]          = {.opcode = CP_E,  .instr_fmt = "CP A, E",       .size = 1},
    [CP_H]          = {.opcode = CP_H,  .instr_fmt = "CP A, H",       .size = 1},
    [CP_L]          = {.opcode = CP_L,  .instr_fmt = "CP A, L",       .size = 1},
    [CP_MEM_HL]         = {.opcode = CP_MEM_HL,     .instr_fmt = "CP A, (HL)",       .size = 1},
    [CP_A]          = {.opcode = CP_A,  .instr_fmt = "CP A, A",       .size = 1},
    [RET_NZ]        = {.opcode = RET_NZ,    .instr_fmt = "RET NZ",      .size = 1},
    [POP_BC]        = {.opcode = POP_BC,    .instr_fmt = "POP BC",      .size = 1},
    [JNZ]           = {.opcode = JNZ,   .instr_fmt = "JNZ 0x%04x",      .size = 3},
    [JMP]           = {.opcode = JMP,   .instr_fmt = "JMP 0x%04x",      .size = 3},
    [CALL_NZ]       = {.opcode = CALL_NZ,   .instr_fmt = "CALL NZ 0x%04x",         .size = 3},
    [PUSH_BC]       = {.opcode = PUSH_BC,   .instr_fmt = "PUSH BC",         .size = 1},
    [ADD_n]         = {.opcode = ADD_n,     .instr_fmt = "ADD A, 0x%02x",      .size = 2},
    [RST_00]        = {.opcode = RST_00,    .instr_fmt = "RST 00",      .size = 1},
    [RET_Z]         = {.opcode = RET_Z,     .instr_fmt = "RET Z",      .size = 1},
    [RET]           = {.opcode = RET,   .instr_fmt = "RET",      .size = 1},
    [JZ]            = {.opcode = JZ,    .instr_fmt = "JZ 0x%04x",       .size = 3},
    [CB_PREFIX]         = {.opcode = CB_PREFIX,     .instr_fmt = "PREFIX OP 0x%02x",       .size = 2},
    [CALL_Z]        = {.opcode = CALL_Z,    .instr_fmt = "CALL Z 0x%04x",         .size = 3},
    [CALL]          = {.opcode = CALL,  .instr_fmt = "CALL 0x%04x",         .size = 3},
    [ADC_N]         = {.opcode = ADC_N,     .instr_fmt = "ADC A, 0x%02x",      .size = 2},
    [RST_08]        = {.opcode = RST_08,    .instr_fmt = "RST 08",      .size = 1},
    [RET_NC]        = {.opcode = RET_NC,    .instr_fmt = "RET NC",      .size = 1},
    [POP_DE]        = {.opcode = POP_DE,    .instr_fmt = "POP DE",      .size = 1},
    [JNC]           = {.opcode = JNC,   .instr_fmt = "JNC 0x%04x",      .size = 3},
    [CALL_NC]       = {.opcode = CALL_NC,   .instr_fmt = "CALL NC 0x%04x",         .size = 3},
    [PUSH_DE]       = {.opcode = PUSH_DE,   .instr_fmt = "PUSH DE",         .size = 1},
    [SUB_n]         = {.opcode = SUB_n,     .instr_fmt = "SUB A, 0x%02x",      .size = 2},
    [RST_10]        = {.opcode = RST_10,    .instr_fmt = "RST 10",      .size = 1},
    [RET_C]         = {.opcode = RET_C,     .instr_fmt = "RET C",      .size = 1},
    [RETI]          = {.opcode = RETI,  .instr_fmt = "RETI",         .size = 1},
    [JC]            = {.opcode = JC,    .instr_fmt = "JC 0x%04x",       .size = 3},
    [CALL_C]        = {.opcode = CALL_C,    .instr_fmt = "CALL C 0x%04x",         .size = 3},
    [SBC_n]         = {.opcode = SBC_n,     .instr_fmt = "SBC A, 0x%02x",      .size = 2},
    [RST_18]        = {.opcode = RST_18,    .instr_fmt = "RST",      .size = 1},
    [STR_DIR_n]         = {.opcode = STR_DIR_n,     .instr_fmt = "STR (0xFF00+0x%02x), A",      .size = 2},
    [POP_HL]        = {.opcode = POP_HL,    .instr_fmt = "POP HL",      .size = 1},
    [STR_DIR]       = {.opcode = STR_DIR,   .instr_fmt = "STR (0xFF00+C), A",      .size = 1},
    [PUSH_HL]       = {.opcode = PUSH_HL,   .instr_fmt = "PUSH HL",         .size = 1},
    [AND_n]         = {.opcode = AND_n,     .instr_fmt = "AND A, 0x%02x",      .size = 2},
    [RST_20]        = {.opcode = RST_20,    .instr_fmt = "RST 20",      .size = 1},
    [ADD_SP]        = {.opcode = ADD_SP,    .instr_fmt = "ADD A, 0x%02x",      .size = 2},
    [JMP_HL]        = {.opcode = JMP_HL,    .instr_fmt = "JMP HL",      .size = 1},
    [LD_MEM_A]      = {.opcode = LD_MEM_A,  .instr_fmt = "LD (0x%04x), A",       .size = 3},
    [XOR_n]         = {.opcode = XOR_n,     .instr_fmt = "XOR A, 0x%02x",      .size = 2},
    [RST_28]        = {.opcode = RST_28,    .instr_fmt = "RST 28",      .size = 1},
    [LD_DIR_n]      = {.opcode = LD_DIR_n,  .instr_fmt = "LD A, (0xFF00+0x%02x)",       .size = 2},
    [POP_AF]        = {.opcode = POP_AF,    .instr_fmt = "POP AF",      .size = 1},
    [LD_DIR]        = {.opcode = LD_DIR,    .instr_fmt = "LD A, (0xFF00+C)",       .size = 1},
    [DI]            = {.opcode = DI,    .instr_fmt = "DI",       .size = 1},
    [PUSH_AF]       = {.opcode = PUSH_AF,   .instr_fmt = "PUSH AF",         .size = 1},
    [OR_n]          = {.opcode = OR_n,  .instr_fmt = "OR A, 0x%02x",       .size = 2},
    [RST_30]        = {.opcode = RST_30,    .instr_fmt = "RST 30",      .size = 1},
    [LD_HL_SP_e]        = {.opcode = LD_HL_SP_e,    .instr_fmt = "LD HL, SP+0x%02x",       .size = 2},
    [LD_SP_HL]      = {.opcode = LD_SP_HL,  .instr_fmt = "LD SP, HL",       .size = 1},
    [LD_A_MEM]      = {.opcode = LD_A_MEM,  .instr_fmt = "LD A, (0x%04x)",       .size = 3},
    [EI]            = {.opcode = EI,    .instr_fmt = "EI",       .size = 1},
    [CP_n]          = {.opcode = CP_n,  .instr_fmt = "CP A, 0x%02x",       .size = 2},
    [RST_38]        = {.opcode = RST_38,    .instr_fmt = "RST 38",      .size = 1},
};
#endif
