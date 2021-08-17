CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= server.out client.out clienttest.out
SERVEROBJS:= FifoList.o files.o server.o generalList.o request.o tree.o utils.o clientTable.o logFun.o
CLIENTOBJ:= api.o utils.o client.o
CLIENT2OBJ:= api.o utils.o testClient.o
OBJS:= main.o
MAINOBJS:= utils.o generalList.o main.o request.o


all: $(TARGETS)
	mv *.out ./bin 
	rm -f *.o

test.out : $(MAINOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	mv *.out ./bin
	mv *.o ./bin

server.out : $(SERVEROBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

client.out : $(CLIENTOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)	

clienttest.out : $(CLIENT2OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o : src/*/%.c
	$(CC) $(CFLAGS) $< -c 

clean : 
	-rm *.o *.out
	-rm ./bin/* 