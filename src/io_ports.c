#include <io_ports.h>
#include <common.h>

/*
 * This file will contain the callback functions for io_ports that have self contained data.
 * i.e. they do not need to access any other pieces of hardware to preform their full function
 */

char SB_data = 0; //hack for writting to console

byte read_joycon(){
    printf("joycon not implemented\n");
    exit(0);
    return 0;
}

void write_joycon( byte data){
    printf("joycon not implemented\n");
    exit(0);
}

byte read_SB(){
    return SB_data;
}

void write_SB(byte data){
    SB_data = data;
}

void write_SC(byte data){
    if(data == 0x81)
        printf("%c", SB_data);
}
