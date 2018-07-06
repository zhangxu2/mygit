#------------------------------------------------------------------------------#
#  version : v2.3
#   author : wangkun
#   create : 2013-05-07
#   module : eswitch admin
#     name : swadmin
# function : start stop eswitch process and do some management stuff.
#   notice :
# modified :
#   author :
#     date :
#  content :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

USERLIB=-lswbase -losbase -ldl
TARGET = swminfo
OBJS =   swminfo.o 

ALL:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB) 
	rm -f $(BINDIR)/$@
	mv $@ $(BINDIR)
	@rm -f *.o $(TARGET)
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o $(TARGET)
