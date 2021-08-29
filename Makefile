CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= server.out clienttest.out ilClient.out
SERVEROBJS:= FifoList.o files.o server.o generalList.o request.o tree.o utils.o clientTable.o logFun.o
ILCLIENTOBJ:= api.o utils.o ilClient.o
CLIENT2OBJ:= api.o utils.o testClient.o

tutto: 
	mv ./obj/*.o ./
	make all

all: $(TARGETS)
	mv *.out ./bin
	mv *.o ./obj
#	rm -f *.o

server.out : $(SERVEROBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ilClient.out : $(ILCLIENTOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clienttest.out : $(CLIENT2OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o : src/*/%.c
	$(CC) $(CFLAGS) $< -c 

test1 : 
	chmod +x creafileA.sh
	./creafileA.sh
	valgrind --leak-check=full ./bin/server.out ./servWork/config_test1 > serverOut &
# -flush&>> servPrint &
	chmod +x clientTest1.sh
	./clientTest1.sh

clean : 
	-rm ./obj/*.o *.out
	-rm ./bin/* 
	-rm -r ./fileXtestLRU
	-rm -r ./filePerTest
	-rm ./servWork/*log 
	-rm ./servWork/*socket*
	-rm -r ./resTest
	-rm -r ./downloaded