#include "main.h"

int main(int argc, char* argv[]){
    //reserve this section for parsing global arguments from argv
    if(argc < 2) 
        LOG(ERROR, "Please specify ROM");

    //argv[1] contains the rom file name
    create_emulator(argv[1]);
    run();

    return 0;
}
