#include <io_ports.h>
#include <common.h>

/*
 * This file will contain the callback functions for io_ports that have self contained data.
 * i.e. they do not need to access any other pieces of hardware to preform their full function
 */

char SB_data = 0; //hack for writting to console

byte read_joycon(void *self){
    io_reg *temp = self;
    return temp->storage;
}

void write_joycon(void *self, byte data){
    io_reg *temp = self;
    temp->storage |= data & 0xF0;
}

byte read_SB(void *self){
    return SB_data;
}

void write_SB(void *self, byte data){
    LOG(INFO, "WRITE SB");
    //getchar();
    SB_data = data;
}

void write_SC(void *self, byte data){
    LOG(INFO, "WRITE SC");
    //getchar();
    if(data == 0x81)
        printf("%c", SB_data);
    return;
}
