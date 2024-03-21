#include "cpu.h"

extern CPU_t cpu;

#define PREF_OP(op, reg) ((op << 3) & reg)
#define BIT_OP(base, bit, reg) (base & ((op << 3) & reg))

int test_ld();
int test_mem();
int test_arith();
int push_pop();
int call_ret();
int test6();

