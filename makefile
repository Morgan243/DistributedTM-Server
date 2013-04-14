#need to add support for cross compile
COMP=g++
CROSS_PPC=powerpc-linux-g++

SERVER_PATH=../NetComm.git/
CACHE_PATH=../sw_access_cache.git/

#we'll want to try and use C11 threads if the cross compiler can do it
FLAGS=-lpthread -ggdb
OUT=test

x86: all_x86

ppc: all_ppc


all_x86 : main_x86.o TM_Server_x86.o  $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o $(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o
	$(COMP) main.o TM_Server.o $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o \
		$(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o $(FLAGS) -o $(OUT)_x86

main_x86.o : main.cpp 
	$(COMP) -c main.cpp $(FLAGS)

TM_Server_x86.o : TM_Server.cpp TM_Server.h 
	$(COMP) -c TM_Server.cpp TM_Server.h $(FLAGS)


all_ppc : main_ppc.o TM_Server_ppc.o  $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o $(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o
	$(CROSS_PPC) main.o TM_Server.o $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o \
		$(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o $(FLAGS) -o $(OUT)_ppc

main_ppc.o : main.cpp 
	$(CROSS_PPC) -c main.cpp $(FLAGS)

TM_Server_ppc.o : TM_Server.cpp TM_Server.h 
	$(CROSS_PPC) -c TM_Server.cpp TM_Server.h $(FLAGS)

clean :
	rm $(OUT)_x86 $(OUT)_ppc *.o *.gch
