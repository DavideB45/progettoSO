CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread

.PHONY: all clean

TARGETS:= server.out clienttest.out ilClient.out
SERVEROBJS:= FifoList.o files.o server.o generalList.o request.o tree.o utils.o clientTable.o logFun.o
ILCLIENTOBJ:= api.o utils.o ilClient.o
CLIENT2OBJ:= api.o utils.o testClient.o


all: $(TARGETS)
	mv *.out ./bin
	mv *.o ./obj

file:
	chmod +x ./zscript/creafileA.sh
	./zscript/creafileA.sh
	chmod +x ./zscript/creafileB.sh
	./zscript/creafileB.sh

server.out : $(SERVEROBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ilClient.out : $(ILCLIENTOBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clienttest.out : $(CLIENT2OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o : src/*/%.c
	$(CC) $(CFLAGS) $< -c 

test1 :
	valgrind --leak-check=full ./bin/server.out ./servWork/config_test1 > serverOut &
	chmod +x ./zscript/clientTest1.sh
	./zscript/clientTest1.sh

test2 :
	./bin/server.out ./servWork/config_test2 > serverOut &
	chmod +x ./zscript/clientTest2.sh
	./zscript/clientTest2.sh

test3 :
	./bin/server.out ./servWork/config_test3 > serverOut &
	chmod +x ./zscript/clientTest3.sh
	./zscript/clientTest3.sh

cleanall : clean
	-rm -r fileXtestLRU
	-rm -r filePerTest

clean : 
	-rm ./obj/*.o *.out
	-rm ./bin/* 
	-rm ./servWork/*log 
	-rm ./servWork/*socket*
	-rm -r resTest
	-rm -r expelled
	-rm serverOut