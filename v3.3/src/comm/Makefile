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

USERLIB = -losbase -lswbase -ldl
SLIB	= libcomm.so

OBJS	= dfs_pool.o \
	  dfs_shmem.o \
	  dfs_shmtx.o \
	  dfs_spool.o \
	  dfs_plist.o \
	  dfs_sem.o \
	  dfs_slab.o 

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
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so
