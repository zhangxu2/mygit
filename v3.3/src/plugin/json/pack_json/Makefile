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

USERLIB = -lswbase -lswpack -lswjsonc -ldl

SLIB	= libswpackjson.so

OBJS	= json_in.o json_out.o

all: $(SLIB) clean
$(SLIB):$(OBJS)
	$(LD) -o $@ $(OBJS) -L$(LIBDIR) $(USERLIB) 
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -I. $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so
