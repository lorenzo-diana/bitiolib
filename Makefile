#CCFLAGS = -O3 -Wall -Werror
CCFLAGS = -O0 -g

all: bitio.o
clean:
	rm bitio.o

bitio.o: bitio.h bitio.c
	cc $(CCFLAGS) -c bitio.c
