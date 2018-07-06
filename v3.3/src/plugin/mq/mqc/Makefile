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

MQINCLUDEDIR=$(MQDIR)/inc
MQLIBDIR=$(MQDIR)/$(MQLIB)
USERLIB_MQ=-lmqm

USRLIB = -lswbase -lswlsn -lmqpub

TARGET1= mqla.so
OBJS1 = mq_pub.o mqla.o

TARGET2 = mqcall.so
OBJS2 = mq_pub.o mqcall.o

TARGET3 = mqla_comm.so
OBJS3 = mq_pub.o mqla_comm.o

TARGETS = $(TARGET1) $(TARGET2) $(TARGET3)
all: $(TARGETS) clean

$(TARGET1):$(OBJS1)
	$(LD) -o $@ $(OBJS1) $(APPLIBDIR) -L$(PLGDIR)  $(USRLIB) -L$(MQLIBDIR) $(USERLIB_MQ)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET2):$(OBJS2)
	$(LD) -o $@ $(OBJS2) $(APPLIBDIR) -L$(PLGDIR)  $(USRLIB) -L$(MQLIBDIR) $(USERLIB_MQ)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET3):$(OBJS3)
	$(LD) -o $@ $(OBJS3) $(APPLIBDIR) -L$(PLGDIR)  $(USRLIB) -L$(MQLIBDIR) $(USERLIB_MQ)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(MQINCLUDEDIR)  -c $*.c

clean:
	rm -f *.*o

