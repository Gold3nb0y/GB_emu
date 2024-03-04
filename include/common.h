#ifndef COMMON_H
#define COMMON_H
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include "log.h"
#include <sys/types.h>
#include <sys/mman.h>


//offer these up to the entire system
void* Malloc(ssize_t size);
uint8_t* Mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);

typedef uint16_t address;
typedef uint8_t byte;

#define SUCCESS 0;
#define FAIL -1;
#define TEST
#define DEBUG_CPU

#endif // !COMMON_H
