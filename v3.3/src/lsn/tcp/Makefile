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

PLGDIR = $(SWHOME)/plugin

USERLIB = -lswbase -lswpack -lswlsn -ldl
TARGETS = 	tcpss.so \
		tcpsc.so \
		tcplc.so \
		tcpla.so \
		tcpcall.so \
		tcpss_ext.so \
		tcpsc_ext.so

all: $(TARGETS) clean

.SUFFIXES: .c .o .so
.c.so:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c
	$(LD) -o $*.so $*.o $(APPLIBDIR) $(USERLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"
clean:
	rm -f *.*o

