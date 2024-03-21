#include "tests.h"
#include "common.h"
#include "opcodes.h"

static bool val_equals_8bitregs(byte val){
    if(cpu.A != val || cpu.B != val || cpu.C != val || cpu.D != val || cpu.E != val || cpu.H != val || cpu.L != val)
        return false;
    return true;
}

int test_ld(){
    int rc = 0;
    char bytecode[] = {
        LD_A, 0x41,
        LD_B, 0x42,
        LD_C, 0x43,
        LD_D, 0x44,
        LD_E, 0x45,
        LD_H, 0x46,
        LD_L, 0x47,
        LD_A, 0,
        LD_B_A,
        LD_C_A,
        LD_D_A,
        LD_E_A,
        LD_H_A,
        LD_L_A,
        LD_B, 1,
        LD_A_B,
        LD_C_B,
        LD_D_B,
        LD_E_B,
        LD_H_B,
        LD_L_B,
        LD_C, 2,
        LD_A_C,
        LD_B_C,
        LD_D_C,
        LD_E_C,
        LD_H_C,
        LD_L_C,
        LD_BC, 0x13, 0x37,
        LD_DE, 0x13, 0x37,
        LD_HL, 0x13, 0x37,
        LD_SP, 0x13, 0x37,
    };

    LOG(DEBUG, "Testing load 8 byte");
    patch(bytecode, sizeof(bytecode));
    exec_program(7);
    if(cpu.A != 0x41 || cpu.B != 0x42 || cpu.C != 0x43 || cpu.D != 0x44 || cpu.E != 0x45 || cpu.H != 0x46 || cpu.L != 0x47){
        dump_cpu();
        LOG(ERROR, "Load PC failed");
        rc = 1;
        goto fail;
    }

    exec_program(7);
    if(!val_equals_8bitregs(0)){
        dump_cpu();
        LOG(ERROR, "Load reg from A failed");
        rc = 2;
        goto fail;
    }

    exec_program(7);
    if(!val_equals_8bitregs(1)){
        dump_cpu();
        LOG(ERROR, "Load reg from B failed");
        rc = 3;
        goto fail;
    }

    exec_program(7);
    if(!val_equals_8bitregs(2)){
        dump_cpu();
        LOG(ERROR, "Load reg from C failed");
        rc = 4;
        goto fail;
    }

    exec_program(4);
    if(cpu.BC != 0x1337 || cpu.DE != 0x1337 || cpu.HL != 0x1337 || cpu.SP != 0x1337){
        dump_cpu();
        LOG(ERROR, "Load 16bit reg from PC failed");
        rc = 5;
        goto fail;
    }

fail:
    return rc;
}

int test_mem(){
    int rc = 0;
    char bytecode[] = {
        LD_BC, 0x13, 0x37, //enable RAM
        LD_A, 0x0A,
        STR_BC,

        LD_BC, 0xA3, 0x37, 
        LD_A, 0x41,
        STR_BC,
        INC_BC,
        LD_A, 0x50,
        STR_BC,
        LD_D_B,
        LD_E_C,
        DEC_DE,
        LD_A, 0,
        LD_A_DE,

        INC_DE,
        LD_A_DE,

        LD_BC, 0x83, 0x37, 
        LD_A, 0x90,
        STR_BC,
        LD_A, 0,
        LD_A_BC,
    };

    patch(bytecode, sizeof(bytecode));

    exec_program(14);
    if(cpu.A != 0x41){
        dump_cpu();
        LOG(ERROR, "Load or store from reg error");
        rc = 1;
        goto fail;
    }

    exec_program(2);
    if(cpu.A != 0x50){
        dump_cpu();
        LOG(ERROR, "Load or store from reg error");
        rc = 2;
        goto fail;
    }

    exec_program(5);
    if(cpu.A != 0x90){
        dump_cpu();
        LOG(ERROR, "read/write VRAM failed");
        rc = 3;
        goto fail;
    }

fail:
    return rc;
}

int test_arith(){
    int rc = -1;
    char bytecode[] = {
        //add check
        LD_A, 0x0,
        LD_B, 0x40,
        ADD_B,
        ADD_B,
        ADD_B,
        ADD_B,

        //half carry check
        LD_B, 0x0f,
        ADD_B,
        ADD_B,

        //sub
        LD_A, 0x0f,
        LD_B, 0x0f,
        SUB_B,
        SUB_B,
        SUB_B,

        //and
        LD_B, 0x1f,
        LD_A, 0xff,
        AND_B,

        LD_B, 0,
        AND_B,

        //xor
        LD_A, 0x3,
        LD_B, 0x2,
        XOR_B,

        XOR_A,

        //or
        LD_A, 0x1,
        LD_B, 0x2,
        OR_B,

        //hc add 16bit
        LD_B, 0xF,
        LD_C, 0xFF,
        LD_H, 0x00,
        LD_L, 0x00,

        ADD_HL_BC,
        ADD_HL_BC,
    };

    //add
    patch(bytecode, sizeof(bytecode));
    exec_program(6);
    if(cpu.A || !cpu.FLAGS.C || !cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "Addition overflow failed");
        rc = 1;
        goto fail;
    }

    //hc add
    exec_program(3);
    if(cpu.A != 0x1e || !cpu.FLAGS.HC){
        dump_cpu();
        LOG(ERROR, "Addition half carry failed");
        rc = 2;
        goto fail;
    }

    exec_program(3);
    if(cpu.A != 0 || !cpu.FLAGS.Z || !cpu.FLAGS.N){
        dump_cpu();
        LOG(ERROR, "sub zero failed");
        rc = 3;
        goto fail;
    }

    exec_program(1);
    if(cpu.A != 0xf1 || !cpu.FLAGS.C || !cpu.FLAGS.N){
        dump_cpu();
        LOG(ERROR, "sub carry failed");
        rc = 4;
        goto fail;
    }

    exec_program(1);
    if(cpu.A != 0xe2 || !cpu.FLAGS.HC || !cpu.FLAGS.N){
        dump_cpu();
        LOG(ERROR, "sub halfcarry failed");
        rc = 5;
        goto fail;
    }
    
    exec_program(3);
    if(cpu.A != 0x1f || cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "and test failed");
        rc = 6;
        goto fail;
    }

    exec_program(2);
    if(cpu.A != 0 || !cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "and self failed");
        rc = 7;
        goto fail;
    }

    exec_program(3);
    if(cpu.A != 1 || cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "xor failed");
        rc = 8;
        goto fail;
    }

    exec_program(1);
    if(cpu.A != 0 || !cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "xor self failed");
        rc = 9;
        goto fail;
    }

    exec_program(3);
    if(cpu.A != 3 || cpu.FLAGS.Z){
        dump_cpu();
        LOG(ERROR, "or failed");
        rc = 10;
        goto fail;
    }

    exec_program(5);
    if(cpu.HL != 0xFFF || cpu.FLAGS.HC){
        dump_cpu();
        LOG(ERROR, "or failed");
        rc = 11;
        goto fail;
    }

    exec_program(1);
    if(cpu.HL != 0x1FFE || !cpu.FLAGS.HC){
        dump_cpu();
        LOG(ERROR, "or failed");
        rc = 12;
        goto fail;
    }

    dump_cpu();
    rc = 0;
fail:
    return rc;
}

int push_pop(){
    int rc = -1;
    char bytecode[] = {
        LD_A, 0x41,
        LD_B, 0x42,
        LD_C, 0x43,
        PUSH_AF,
        PUSH_BC,
        POP_DE,
        POP_HL,
    };

    patch(bytecode, sizeof(bytecode));
    exec_program(7);

    if(cpu.DE != 0x4243 || cpu.HL != 0x4100){
        dump_cpu();
        LOG(ERROR, "push pop failed");
        rc = 1;
        goto fail;
    }

    dump_cpu();
    rc = 0;
fail:
    return rc;
}
int call_ret(){
    int rc = -1;
    char bytecode[] = {
        LD_A, 0x41,
        LD_B, 0x42,
        LD_C, 0x43,
        PUSH_AF,
        PUSH_BC,
        CALL, 0x1, 0x10,
        POP_DE,
        NOP,
        NOP,
        NOP,
        NOP,
        LD_H, 0x41,
        LD_L, 0x42,
        RET,
    };

    patch(bytecode, sizeof(bytecode));
    exec_program(6);

    if(cpu.PC != 0x110){
        dump_cpu();
        LOG(ERROR, "CALL failed");
        rc = 1;
        goto fail;
    }
        
    exec_program(2);
    if(cpu.HL != 0x4142){
        dump_cpu();
        LOG(ERROR, "After CALL failed");
        rc = 2;
        goto fail;
    }

    exec_program(1);
    if(cpu.PC != 0x10b){
        dump_cpu();
        LOG(ERROR, "RET failed");
        rc = 2;
        goto fail;
    }

    exec_program(1);
    if(cpu.DE != 0x4243){
        dump_cpu();
        LOG(ERROR, "stack offed");
        rc = 2;
        goto fail;
    }

    dump_cpu();
    rc = 0;
fail:
    return rc;
}

int prefixed_instr(){
    int rc = -1;
    char bytecode[] = {
        LD_A, 0x41,
        CB_PREFIX, (RLC << 4) & A,
    };

    patch(bytecode, sizeof(bytecode));
    exec_program(6);

fail:
    return rc;
}

