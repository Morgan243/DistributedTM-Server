#need to add support for cross compile
COMP=g++

SERVER_PATH=../NetComm.git/
CACHE_PATH=../sw_access_cache.git/

#we'll want to try and use C11 threads if the cross compiler can do it
FLAGS=-lpthread --std=c++11 -ggdb
OUT=test


all : main.o TM_Server.o  $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o $(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o
	$(COMP) main.o TM_Server.o $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o \
		$(CACHE_PATH)/RWStore.o $(CACHE_PATH)/AccessCache.o $(FLAGS) -o $(OUT)

main.o : main.cpp 
	$(COMP) -c main.cpp $(FLAGS)

TM_Server.o : TM_Server.cpp TM_Server.h 
	$(COMP) -c TM_Server.cpp TM_Server.h $(FLAGS)

clean :
	rm $(OUT) *.o *.gch
