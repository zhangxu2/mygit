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

USERLIB = -losbase -lswbase -lpthread -ldl $(SOCKET) $(NSL)

TARGET = swalert

OBJS = alert_proc.o 

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(BINDIR)/$@ 
	@rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o $(TARGET)
