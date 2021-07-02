CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= davide.out 
OBJS:= main.o include.o


all: $(TARGETS)


davide.out : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o : src/%.c
	$(CC) $(CFLAGS) $< -c 

clean : 
	-rm *.o *.out