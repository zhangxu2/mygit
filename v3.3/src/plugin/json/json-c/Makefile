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


OBJS = arraylist.o json_c_version.o json_object_iterator.o json_util.o linkhash.o random_seed.o \
	debug.o json_object.o json_tokener.o libjson.o printbuf.o

TARGET = libswjsonc.so

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(LD) -o $@ $(OBJS) -lm
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv -f $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	echo "$@ is ok!"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	rm -fr *.o core.* 
