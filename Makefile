###This make file was intended for use with nmake

#compiler
CC = cl

#executable name
EXE = CFS

#include directory
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

#source files
SRC = $(SRC_DIR)/*.c

#flags
CFLAGS = /W3 /nologo /D_CRT_SECURE_NO_WARNINGS

.PHONY: all

all: $(EXE)

$(EXE): $(SRC)
	$(CC) $(CFLAGS) /I$(INCLUDE_DIR) $? /Fo: $(OBJ_DIR)/ /Fe: $@.exe


clean:
	del $(OBJ_DIR)\*.obj $(EXE).exe