
include $(SWHOME)/make/sw.env

include $(SWHOME)/make/sw.env
TONGEASYHOMEDIR=$(TEADIR)
TELIBDIR=$(TONGEASYHOMEDIR)/lib/libtesvr_NOXA.a
#TELIBDIR=$(TONGEASYHOMEDIR)/lib/teapilib64
TEINCLDIR=$(TONGEASYHOMEDIR)/incl

USERLIB = -losbase -lswpack  -lswlsn -lswbase -ldl
TARGET = teasc.so

all:   $(TARGET) clean

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(TEINCLDIR) -c $*.c
.o.so:
	$(CC) $(GFLAGS) -o $*.so $*.o $(APPLIBDIR) $(USERLIB) $(TELIBDIR)
	rm -f $(PLGDIR)/$@
	rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

clean:
	rm -f *.o
