#------------------------------------------------------------------------------#
#  version : v3.0
#   author : MaWeiwei
#   create : 2013-12-01
#   module : Trace Monitor
#     name : 
# function :
#   notice :
# modified :
#   author :
#     date :
#  content :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

TARGET = txview
OBJS = txview.o ListWin.o InputWin.o CWin.o
ALL:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o  $@ $(OBJS) $(APPLIBDIR) -lswbase -lcurses -lC
	cp -f $@ $(BINDIR)/$@
	mv -f $@ $(BINDIR)/lc
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .cpp .c .o
.cpp.o:
	$(CC) $(CFLAGS) -I/usr/include $(APPINCLUDE) -c $*.cpp
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o $(TARGET)
