CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= davide.out server.out api.out
SERVEROBJS:= FifoList.o files.o server.o
APIOBJ:=
OBJS:= main.o include.o


all: $(TARGETS)

server.out : $(SERVEROBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

davide.out : $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o : src/*/%.c
	$(CC) $(CFLAGS) $< -c 

clean : 
	-rm *.o *.out