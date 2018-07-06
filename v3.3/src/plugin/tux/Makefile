#------------------------------------------------------------------------------#
#  VERSION: V3.0
#   AUTHOR: CHANNEL DEVELOPMENT TEAM
#     DATE: 2013-07-01
#   MODULE:
#     NAME:
#   NOTICE:
# MODIFIED:
#   AUTHOR:
#     DATE:
#  CONTENT:
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

TUXINCL = -I$(TUXDIR)/include
TUXLIB = -L$(TUXDIR)/lib


USERLIB_TUX = -lwsc -lbuft -lwsc  -lgpnet  -lfml -lfml32 -lengine  -ldl -lpthread
#USERLIB_TUX = $(TUXDIR)/lib/wstpinit.o  -lwsc -lbuft -lwsc  -lgpnet  -lfml -lfml32 -lengine  -lpthread

USERLIB = -lswpack  -lswlsn -lswbase -ldl
TARGETS = tuxsc.so \
	  tuxlc.so \
	  tuxcall.so

all:   $(TARGETS) clean

.SUFFIXES: .c .o .so
.c.so:
	$(CC) $(CFLAGS) $(APPINCLUDE) $(TUXINCL) -c $*.c
	$(LD) -o $*.so $*.o $(APPLIBDIR) $(TUXLIB) $(USERLIB) $(USERLIB_TUX)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"
clean:
	rm -f *.*o

