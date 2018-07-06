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
MQLIBDIR=$(MQDIR)/lib64
USERLIB_MQ=-lmqm

USRLIB = -lswbase -lswlsn -lswpack -L$(JVMLIBDIR) -ljvm -L$(NIOLIBDIR) -lnio

TARGET = jmqtest
OBJS = jmqtest.o

TARGETS = $(TARGET)
all: $(TARGETS) clean
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USRLIB)
	@echo "$@ is OK !"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I$(MQINCLUDEDIR) -I../  -I$(SWHOME)/src/plugin/mq -c $*.c

clean:
	rm -f *.*o

