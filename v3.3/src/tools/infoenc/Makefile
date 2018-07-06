#------------------------------------------------------------------------------#
#  version : v1.0
#   author : zhangxu
#   create : 2017-06-20
#     name : swinfoenc
# function : swinfoenc
#   notice :
# modified :
#   author :
#     date :
#  content :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

USERLIB=-lswbase -losbase -ldl
TARGET = swinfoenc
OBJS =   swinfoenc.o

ALL:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	rm -f $(BINDIR)/$@
	rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	@rm -f *.o $(TARGET)
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o $(TARGET)
