
include $(SWHOME)/make/sw.env
ECC = $(INFORMIXDIR)/bin/esql
INCLUDE = -I.\
		-I$(INFORMIXDIR)/incl/esql \
		-I$(INFORMIXDIR)/incl/public \
		-I$(SWHOME)/src/incl_pub

LIB_PATH = -L$(INFORMIXDIR)/lib -L$(INFORMIXDIR)/lib/esql

ELIBS = `esql -libs` -lswbase

OBJS = db_ifx_opt.o
TARGET = libswifx.so

.PHONY: all clean

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(CC) $(GFLAGS) -o $(TARGET) $(OBJS) $(EOBJS) $(APPLIBDIR) $(LIB_PATH) $(INCLUDE) $(ELIBS)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv -f $@ $(LIBDIR)/$@.$(VERSION)
	ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .ec .c .o

.ec.o:
	$(ECC) $(CFLAGS) $(INCLUDE) -c $*.ec

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $*.c


clean: 
	rm -fr $(OBJS) $(TARGET) db_ifx_opt.c  
