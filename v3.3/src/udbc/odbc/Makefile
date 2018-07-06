
include $(SWHOME)/make/sw.env

INCLUDE = -I. -I$(SWHOME)/src/incl_pub -I$(DB2_HOME)/include
LIB_PATH = -L$(SWHOME)/lib -L$(DB2_HOME)/$(DB2LIB)

LIBS = -ludbc -ldl -lswbase -ldb2

OBJS = db_odbc_opt.o
TARGET = libswodbc.so

.PHONY: all clean

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(CC) $(GFLAGS) -o $(TARGET) $(OBJS) $(INCLUDE) $(LIB_PATH) $(LIBS)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv -f $@ $(LIBDIR)/$@.$(VERSION)
	ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $*.c


clean: 
	rm -fr $(OBJS) $(TARGET)