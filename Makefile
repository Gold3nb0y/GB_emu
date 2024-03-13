#define a series of objects in an array
OBJS := main.o build/mapper.o build/main_bus.o build/common.o build/emulator.o build/log.o build/cart.o build/cpu.o
CC = gcc
CFLAGS = -g -Wall -I./include/ 

#comment to build without tests
test = true

ifdef test
OBJS += build/tests.o
CFLAGS += -D TEST -fprofile-arcs -ftest-coverage
LFLAGS := -lgcov --coverage
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
