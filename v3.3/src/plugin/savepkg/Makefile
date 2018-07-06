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

LTARGETS = libsavepkg.so
TARGETS = libsavepkg.so.$(VERSION)

OBJS = savepkg.o

all: $(TARGETS)
$(TARGETS): $(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	mv $@ $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$(LTARGETS)
	@ln -s $@ $(PLGDIR)/$(LTARGETS)
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so
