CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= server.out api.out
SERVEROBJS:= FifoList.o files.o server.o 
APIOBJ:= api.o utils.o
OBJS:= main.o


all: $(TARGETS)
	mv *.out ./bin 
	rm -f *.o

server.out : $(SERVEROBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

api.out : $(APIOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)	

%.o : src/*/%.c
	$(CC) $(CFLAGS) $< -c 

clean : 
	-rm *.o *.out
	-rm ./bin/* 