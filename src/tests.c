#include "tests.h"
#include "common.h"
#include "opcodes.h"


int test1(){
    char bytecode[] = {LD_A, 0xFF};
    patch(bytecode, 2);
    exec_program(1);
    LOG(INFO, "program completed");
    dump_cpu();
    return 0;
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

