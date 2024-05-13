#include <io_ports.h>
#include <common.h>

/*
 * This file will contain the callback functions for io_ports that have self contained data.
 * i.e. they do not need to access any other pieces of hardware to preform their full function
 */

byte read_joycon(void *self){
    io_reg *temp = self;
    return temp->storage;
}

void write_joycon(void *self, byte data){
    io_reg *temp = self;
    temp->storage |= data & 0xF0;
}
