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

BINDIR = $(SWHOME)/bin
PLGDIR = $(SWHOME)/plugin

USERLIB = -lswpack  -lswlsn -lswbase -ldl -lpthread

OBJS = autosend.o

TARGET = autosend.x

all:   $(TARGET) clean
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(BINDIR)/$@
	mv $@ $(BINDIR)/$@
	echo "$@ is ok!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c -I$(SUNLINKHOME)/include

clean:
	rm -f *.*o

