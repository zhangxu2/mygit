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

USERLIB = -lswbase -ldl

SLIB	= libswpack.so

OBJS	= pack_in.o pack_pub.o pack_out.o pack.o pub_xml_convert.o

all: $(SLIB) clean
$(SLIB):$(OBJS)
	$(LD) -o $@ $(OBJS) -L$(LIBDIR) $(USERLIB) 
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -I. $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so
