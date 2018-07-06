all: test

CC = gcc
#ESQL = $(INFORMIXDIR)/bin/esql

#CFLAGS = -q64 -g -qcpluscmt -brtl
CFLAGS = -Wall -g

INCLUDE = -I$(SWHOME)/src/incl_pub


LIB_PATH = -L$(SWWORK)/txlib -L$(SWHOME)/lib
LIB_LIST = -ludbc -ldl

OBJS = test.o 
TARGET = test

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(INCLUDE) $(LIB_PATH) $(LIB_LIST)
	rm -fr $(OBJS)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $*.c
clean:
	rm -f $(TARGET) $(OBJS) *.so
