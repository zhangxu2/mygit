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

USRLIB = -lpthread  -lswbase

TARGETS = libswdes.so

OBJS = encrypt.o

all: $(TARGETS) clean
$(TARGETS): $(OBJS)
	$(LD) -o $@ $(OBJS) $(APPLIBDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so
