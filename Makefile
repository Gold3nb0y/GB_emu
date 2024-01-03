#define a series of objects in an array
OBJS := main.o build/emulator.o build/log.o build/cart.o build/mapper.o build/main_bus.o
CC = clang
CFLAGS = -g -Wall -I./include/


all: $(OBJS)# all requires the object files to run
	$(CC) -o gameboi $(OBJS) 

#the requires part is important for checking the timestamp
main.o : main.c 
	$(CC) -c $(CFLAGS) main.c -o main.o

build/%.o : src/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm build/* gameboi main.o