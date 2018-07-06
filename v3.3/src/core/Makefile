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
USERLIB= -lpthread -losbase -ldl $(ICONV) $(SOCKET)

SLIB = libswbase.so
SAPILIB = libswapi.so

OBJS =  pub_buf.o  \
	pub_route.o	\
	pub_code.o	\
	pub_errno.o	\
	pub_file.o	\
	pub_hash.o	\
	pub_log.o	\
	pub_mem.o	\
	pub_pool.o	\
	pub_queue.o	\
	pub_rbtree.o	\
	pub_signal.o	\
	pub_string.o	\
	msg_trans.o	\
	pub_time.o	\
	pub_xml.o \
	pub_cfg.o \
	pub_regex.o	\
	pub_xsd.o	\
	shm_slab.o	\
	shm_vars.o	\
	pub_vars.o	\
	pub_comvars.o \
	variable.o  \
	sem_lock.o \
	mtype.o \
	mtype_limit.o \
	seqs.o \
	procs.o \
	trace.o \
	job.o \
	run.o \
	alert.o \
	alias_pool.o  \
	slab.o  \
	thread_pool.o \
	cycle.o \
	common.o \
	param.o \
	prdt_arg.o \
	api.o \
	des3.o \
	pub_hvar.o \
	pub_filter.o \
	pub_stack.o \
	pub_computexp.o \
	pub_minifun.o \
	pub_ares.o \
	anet.o \
	pub_shmex.o \
	pub_alog.o

APIOBJS = variable.o \
	  api.o \
	  pub_xml.o	\
	  pub_string.o \
	  pub_log.o  \
	  anet.o

all: $(SLIB) $(SAPILIB) clean

$(SLIB): $(OBJS)
	$(LD) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@echo "$@ is OK !"

$(SAPILIB): $(APIOBJS)
	$(LD) -o $@ $(APIOBJS) $(APPLIBDIR) $(USERLIB)
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
	@rm -f *.o core trc.log

