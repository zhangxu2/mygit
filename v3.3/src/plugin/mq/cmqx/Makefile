
include $(SWHOME)/make/sw.env

MQINCLUDEDIR=$(MQDIR)/inc
MQLIBDIR=$(MQDIR)/lib64
USERLIB_MQ=-lmqic

USRLIB = -lswbase -lswlsn -lswpack -lmqpub

TARGET1= mqlax.so
OBJS1 =  mqx_pub.o mqlax.o

TARGET3 = mqlax_comm.so
OBJS3 =  mqx_pub.o mqlax_comm.o

TARGETS = $(TARGET1) $(TARGET2) $(TARGET3)
all: $(TARGETS) clean

.PHONY : compiler
compiler:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -c $(CBJS)

$(TARGET1):$(OBJS1)
	$(LD) -o $@ $(OBJS1) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB) -L$(MQLIBDIR) $(USERLIB_MQ)
	rm -f $(PLGDIR)/$@
	rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET3):$(OBJS3)
	$(LD) -o $@ $(OBJS3) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB) -L$(MQLIBDIR) $(USERLIB_MQ)
	rm -f $(PLGDIR)/$@
	rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(MQINCLUDEDIR) -c $*.c
clean:
	rm -f *.*o
