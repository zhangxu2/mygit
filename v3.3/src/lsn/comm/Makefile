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

USERLIB = -lswbase -lswlsn
LUSERLIB = -lpthread -losbase -lswbase -lswpack -lswmap -ludbc -ldl 

TARGET = swlsn

OBJS = lsn_main.o 
SLIB = libswlsn.so

SLIBOBJS = lsn_cache.o lsn_pub.o lsn_cycle.o lsn_comm.o lsn_alink.o lsn_slink.o

all: $(SLIB) $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(BINDIR)/$@
	@rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	@rm -f *.*o
	@echo "$@ is OK !"

$(SLIB):$(SLIBOBJS)
	$(LD) -o $@ $(SLIBOBJS) $(APPLIBDIR) $(LUSERLIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.*o
	@echo "$@ is OK!"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I. -c $*.c

clean:
	@rm -f *.o core trc.log
