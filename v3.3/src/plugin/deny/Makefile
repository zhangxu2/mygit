
include $(SWHOME)/make/sw.env
PLGDIR = $(SWHOME)/plugin

USERLIB = -losbase -lswpack  -lswlsn -lswbase -ldl
SLIB   = libdeny.so 
OBJS   	= deny.o

all: $(SLIB) clean
$(SLIB): $(OBJS)
	$(LD) -o $@ $(OBJS) -L$(LIBDIR) $(USERLIB)
	rm -f $(PLGDIR)/$(SLIB)
	mv $(SLIB) $(PLGDIR)	
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -c $*.c

clean:
	rm -f *.*o

