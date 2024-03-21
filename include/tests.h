#include "cpu.h"
#include <stdint.h>

extern CPU_t cpu;

#define BIT_OP(base, op, reg) ((base) | ((reg) | ((op) << 3)))

int test_ld();
int test_mem();
int test_arith();
int push_pop();
int call_ret();
int prefixed_instr();

