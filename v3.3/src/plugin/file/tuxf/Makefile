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

TUXINCLUDE = -I$(TUXDIR)/include
TUXLIBDIR= -L/usr/local/lib -L$(TUXDIR)/lib

USRLIB = -lswbase -lswlsn -lc -ltux -lbuft  -lfml -lfml32 -lengine -lpthread

TARGETS = libfile.so
OBJS = tux_7f.o tail_7f.o tailm_7f.o

$(TARGETS): $(OBJS)
	$(LD) -o $@ $(OBJS) $(APPLIBDIR) $(TUXLIBDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv -f $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o 
	@echo "$@ is OK !"

.SUFFIXES: .c .cpp .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) $(TUXINCLUDE) -c $*.c
.cpp.o:
	$(CPP)  $(CFLAGS)  $(APPINCLUDE) $(TUXINCLUDE) -c $*.cpp

clean:
	rm -f *.o *.so
