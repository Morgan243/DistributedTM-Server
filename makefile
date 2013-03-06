#need to add support for cross compile
COMP=g++

SERVER_PATH=../NetComm.git/

#we'll want to try and use C11 threads if the cross compiler can do it
FLAGS=-lpthread --std=c++11
OUT=test


all : main.o TM_Server.o  $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o
	$(COMP) main.o TM_Server.o $(SERVER_PATH)NC_Server.o $(SERVER_PATH)/NetComm.o $(FLAGS) -o $(OUT)

main.o : main.cpp 
	$(COMP) -c main.cpp $(FLAGS)

TM_Server.o : TM_Server.cpp TM_Server.h 
	$(COMP) -c TM_Server.cpp TM_Server.h $(FLAGS)

clean :
	rm $(OUT) *.o *.gch
