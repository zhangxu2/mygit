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


USRLIB = -lswbase -lswlsn -lswpack -lmqpub -L$(JVMLIBDIR) -ljvm -L$(NIOLIBDIR) -lnio

TARGET1= jmqla.so
OBJS1 = jmq.o jmqla.o

TARGET2 = jmqla_comm.so
OBJS2 = jmq.o jmqla_comm.o

TARGET3 = jmqcall.so
OBJS3 = jmq.o jmqcall.o
TARGETS = $(TARGET1) $(TARGET2) $(TARGET3)
all: $(TARGETS) clean

$(TARGET1):$(OBJS1)
	$(LD) -o $@ $(OBJS1) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET2):$(OBJS2)
	$(LD) -o $@ $(OBJS2) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET3):$(OBJS3)
	$(LD) -o $@ $(OBJS3) $(APPLIBDIR) -L$(PLGDIR) $(USRLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"
.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux  -c $*.c

clean:
	rm -f *.*o

