CC:= gcc
CFLAGS:= -Wall -pedantic -I ./include
LIBS:= -lpthread -L ./lib -lapi -Wl,-rpath,./lib

.PHONY: all clean lib file

TARGETS:= server.out ilClient.out

LIB_OBJS:= utils.o api.o request.o

SERVEROBJS:= FifoList.o files.o server.o generalList.o request.o tree.o utils.o clientTable.o logFun.o
ILCLIENTOBJ:= utils.o ilClient.o



all: lib $(TARGETS)
	mv *.out ./bin
	mv *.o ./obj

lib :
	$(CC) src/utils/utils.c $(CFLAGS) $< -c -fPIC
	$(CC) src/API/api.c $(CFLAGS) $< -c -fPIC
	$(CC) src/utils/request.c $(CFLAGS) $< -c -fPIC
	$(CC) $(LIB_OBJS) -shared -o libapi.so $^
	mv libapi.so ./lib

file:
	chmod +x ./zscript/creafileA.sh
	./zscript/creafileA.sh
	chmod +x ./zscript/creafileB.sh
	./zscript/creafileB.sh
	chmod +x ./zscript/statistiche.sh

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
	-rm ./obj/*.o *.o *.out
	-rm ./bin/* 
	-rm ./lib/*
	-rm ./servWork/*log 
	-rm ./servWork/*socket*
	-rm -r resTest
	-rm -r expelled
	-rm serverOut