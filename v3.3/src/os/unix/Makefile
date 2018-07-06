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


OBJS = pub_shm.o pub_shm_ex.o pub_fifo.o pub_sem.o pub_msq.o pub_key.o pub_usocket.o select_event.o pub_proc.o

TARGET = libosbase.so

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(LD) -o $@ $(OBJS)
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
