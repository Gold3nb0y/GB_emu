#include "mapper.h"
#include "main_bus.h"

mapper_t* create_mapper(main_bus_t* main_bus){
    mapper_t* mapper;
    mapper = malloc(sizeof(mapper_t));
    mapper->bus = main_bus;

    //to be figured out later
    mapper->read_RAM = NULL;
    mapper->read_ROM = NULL;
    mapper->write_RAM = NULL; 
    return mapper;
}
