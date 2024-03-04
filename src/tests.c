#include "tests.h"
#include "common.h"
#include "opcodes.h"

int test1(){
    int rc = 0;
    char bytecode[] = {
        LD_A, 0x41,
        LD_B, 0x42,
        LD_C, 0x43,
        LD_D, 0x44,
        LD_E, 0x45,
        LD_H, 0x46,
        LD_L, 0x47,
    };
    LOG(DEBUG, "Testing load 8 byte");
    patch(bytecode, sizeof(bytecode));
    exec_program(7);
    LOGF(DEBUG, "cpu @%p", &cpu);
    dump_cpu();
    if(cpu.A != 0x41 || cpu.B != 0x42 || cpu.C != 0x43 || cpu.D != 0x44 || cpu.E != 0x45 || cpu.H != 0x46 || cpu.L != 0x47)
        rc = -1;
    return rc;
}
int test2(){

    return 0;
}
int test3(){

    return 1;
}
int test4(){

    return 0;
}
int test5(){

    return 0;
}

int test6(){

    return 0;
}

