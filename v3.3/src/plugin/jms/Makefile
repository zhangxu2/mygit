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

USRLIB = -lswbase -lswlsn -lswpack -L$(JVMLIBDIR) -ljvm -L$(NIOLIBDIR) -lnio

TLQINCLUDEDIR=$(TLQHOMEDIR)/incl
TLQLIBDIR=$(TLQHOMEDIR)/lib
TLQLIB=-ltl_api

TARGETS = jmsla.so
OBJS = jms_pub.o jmsla.o

TARGETS1 = jmscall.so
OBJS1 = jms_pub.o jmscall.o

all: $(TARGETS) clean

.PHONY : compiler
compiler:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -c $(CBJS)

$(TARGETS):$(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGETS1):$(OBJS1)
	$(CC) $(GFLAGS) -o $@ $(OBJS1) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) -D__SW_USE_JMS__ $(APPINCLUDE) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I$(TLQINCLUDEDIR) -c $*.c

clean:
	rm -f *.*o

