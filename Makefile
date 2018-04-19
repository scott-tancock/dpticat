CC = gcc

CC_FLAGS = --std=c99 -ggdb

CPP = g++

CPP_FLAGS = --std=c++11 -ggdb

INC_FLAGS = -I /usr/include/digilent/adept

LIB_DIRS = -L /lib/digilent/adept

LIBS = -ldmgr -ldpti

all : dpticat

dpticat : dpticat.cpp
	$(CPP) $(CPP_FLAGS) -o $@ $(INC_FLAGS) $^ $(LIB_DIRS) $(LIBS)

clean :
	rm dpticat
