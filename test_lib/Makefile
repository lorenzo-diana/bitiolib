#CCFLAGS = -O3 -Wall -Werror
CCFLAGS = -O0 -g

all: test
clean:
	cd ../ && $(MAKE) clean
	rm test

bitio.o: ../bitio.h ../bitio.c
	cd ../ && $(MAKE) bitio.o

test: test_lib.c bitio.o
	cc $(CCFLAGS) test_lib.c ../bitio.o -o test
