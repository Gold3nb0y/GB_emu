#define a series of objects in an array
OBJS := main.o build/mapper.o build/main_bus.o build/common.o build/lcd.o build/emulator.o build/log.o build/cart.o build/cpu.o build/ppu.o build/io_ports.o
CC = gcc
CFLAGS = -g -Wall -I./include/ -D HEADLESS -D DEBUG_CPU -D DEBUG_BUS
LFLAGS := -lraylib

#comment to build without tests
#test = true

ifdef test
OBJS += build/tests.o
CFLAGS += -D TEST -D HEADLESS -fprofile-arcs -ftest-coverage
LFLAGS += -lgcov --coverage 
endif

all: $(OBJS)# all requires the object files to run
	$(CC) -o gameboi $(OBJS) $(LFLAGS)

#the requires part is important for checking the timestamp
main.o : main.c 
	$(CC) -c $(CFLAGS) main.c -o main.o $(LFLAGS)

build/%.o : src/%.c include/%.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm build/* gameboi main.o main.gc*
