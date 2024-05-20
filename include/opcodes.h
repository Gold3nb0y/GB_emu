#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
enum opcodes{
    NOP = 0,
    LD_BC,
    STR_BC,
    INC_BC,
    INC_B,
    DEC_B,
    LD_B,
    RLCA,
    STR_nn_SP,
    ADD_HL_BC,
    LD_A_BC,
    DEC_BC,
    INC_C,
    DEC_C,
    LD_C,
    RRCA,
    STOP,
    LD_DE,
    STR_DE,
    INC_DE,
    INC_D,
    DEC_D,
    LD_D,
    RLA,
    JR,
    ADD_HL_DE,
    LD_A_DE,
    DEC_DE,
    INC_E,
    DEC_E,
    LD_E,
    RRA,
    JR_NZ,
    LD_HL,
    STRI_HL,
    INC_HL,
    INC_H,
    DEC_H,
    LD_H,
    DAA,
    JR_Z,
    ADD_HL_HL,
    LDI_A_HL,
    DEC_HL,
    INC_L,
    DEC_L,
    LD_L,
    CPL,
    JR_NC,
    LD_SP,
    STRD_HL,
    INC_SP,
    INC_MEM,
    DEC_MEM,
    LD_MEM,
    SCF,
    JR_C,
    ADD_HL_SP,
    LDD_A_HL,
    DEC_SP,
    INC_A,
    DEC_A,
    LD_A,
    CCF,
    LD_B_B,
    LD_B_C,
    LD_B_D,
    LD_B_E,
    LD_B_H,
    LD_B_L,
    LD_B_MEM_HL,
    LD_B_A,
    LD_C_B,
    LD_C_C,
    LD_C_D,
    LD_C_E,
    LD_C_H,
    LD_C_L,
    LD_C_MEM_HL,
    LD_C_A,
    LD_D_B,
    LD_D_C,
    LD_D_D,
    LD_D_E,
    LD_D_H,
    LD_D_L,
    LD_D_MEM_HL,
    LD_D_A,
    LD_E_B,
    LD_E_C,
    LD_E_D,
    LD_E_E,
    LD_E_H,
    LD_E_L,
    LD_E_MEM_HL,
    LD_E_A,
    LD_H_B,
    LD_H_C,
    LD_H_D,
    LD_H_E,
    LD_H_H,
    LD_H_L,
    LD_H_MEM_HL,
    LD_H_A,
    LD_L_B,
    LD_L_C,
    LD_L_D,
    LD_L_E,
    LD_L_H,
    LD_L_L,
    LD_L_MEM_HL,
    LD_L_A,
    LD_MEM_HL_B,
    LD_MEM_HL_C,
    LD_MEM_HL_D,
    LD_MEM_HL_E,
    LD_MEM_HL_H,
    LD_MEM_HL_L,
    HALT,
    LD_MEM_HL_A,
    LD_A_B,
    LD_A_C,
    LD_A_D,
    LD_A_E,
    LD_A_H,
    LD_A_L,
    LD_A_MEM_HL,
    LD_A_A,
    ADD_B,
    ADD_C,
    ADD_D,
    ADD_E,
    ADD_H,
    ADD_L,
    ADD_MEM_HL,
    ADD_A,
    ADC_B,
    ADC_C,
    ADC_D,
    ADC_E,
    ADC_H,
    ADC_L,
    ADC_MEM_HL,
    ADC_A,
    SUB_B,
    SUB_C,
    SUB_D,
    SUB_E,
    SUB_H,
    SUB_L,
    SUB_MEM_HL,
    SUB_A,
    SBC_B,
    SBC_C,
    SBC_D,
    SBC_E,
    SBC_H,
    SBC_L,
    SBC_MEM_HL,
    SBC_A,
    AND_B,
    AND_C,
    AND_D,
    AND_E,
    AND_H,
    AND_L,
    AND_MEM_HL,
    AND_A,
    XOR_B,
    XOR_C,
    XOR_D,
    XOR_E,
    XOR_H,
    XOR_L,
    XOR_MEM_HL,
    XOR_A,
    OR_B,
    OR_C,
    OR_D,
    OR_E,
    OR_H,
    OR_L,
    OR_MEM_HL,
    OR_A,
    CP_B,
    CP_C,
    CP_D,
    CP_E,
    CP_H,
    CP_L,
    CP_MEM_HL,
    CP_A,
    RET_NZ,
    POP_BC,
    JNZ,
    JMP,
    CALL_NZ,
    PUSH_BC,
    ADD_n,
    RST_00,
    RET_Z,
    RET,
    JZ,
    CB_PREFIX,
    CALL_Z,
    CALL,
    ADC_N,
    RST_08,
    RET_NC,
    POP_DE,
    JNC,
    CALL_NC = 0xD4,
    PUSH_DE,
    SUB_n,
    RST_10,
    RET_C,
    RETI,
    JC,
    CALL_C = 0xDC,
    SBC_n = 0xDE,
    RST_18,
    STR_DIR_n,
    POP_HL,
    STR_DIR,
    PUSH_HL = 0xE5,
    AND_n,
    RST_20,
    ADD_SP,
    JMP_HL,
    LD_MEM_A,
    XOR_n = 0xEE,
    RST_28,
    LD_DIR_n,
    POP_AF,
    LD_DIR,
    DI,
    PUSH_AF = 0xF5,
    OR_n,
    RST_30,
    LD_HL_SP_e,
    LD_SP_HL,
    LD_A_MEM,
    EI,
    CP_n = 0xFE,
    RST_38
};

enum prefixed_opcodes{
    RLC = 0,
    RRC,
    RL,
    RR,
    SLA,
    SRA,
    SWAP,
    SRL
};

enum REGS_8 {
    B = 0,
    C,
    D,
    E,
    H,
    L,
    MEM,
    A
};

enum REGS_16 {
    BC = 0,
    DE,
    HL,
    SP,
};

#define ROT 0x0
#define BIT 0x40
#define RES 0x80
#define SET 0xC0

typedef struct{
    uint8_t opcode;
    char instr_fmt[0x18];
    uint8_t size;
} instr;


#endif
