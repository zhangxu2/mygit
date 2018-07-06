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

USRLIB = -lswbase -lswlsn -lc 

TARGETS = libfile.so
OBJS = tail_7f.o tailm_7f.o

$(TARGETS): $(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv -f $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o 
	@echo "$@ is OK !"

.SUFFIXES: .c .cpp .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c
.cpp.o:
	$(CPP)  $(CFLAGS)  $(APPINCLUDE) -c $*.cpp

clean:
	rm -f *.o *.so
