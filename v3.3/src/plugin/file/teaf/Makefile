
include $(SWHOME)/make/sw.env

TONGEASYHOMEDIR=$(TEADIR)
TELIBDIR=$(TONGEASYHOMEDIR)/lib/teapilib64
TEINCLDIR=$(TONGEASYHOMEDIR)/incl

USERLIB = -lswbase -lswlsn -lc
TARGETS = libfile.so
OBJS = tea_7f.o tail_7f.o tailm_7f.o

all: $(TARGETS) clean

$(TARGETS): $(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB) $(TELIBDIR)
	rm -f $(PLGDIR)/$@
	rm -f $(PLGDIR)/$@.$(VERSION)
	mv -f $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(TEINCLDIR) -c $*.c

clean:
	rm -f *.*o
