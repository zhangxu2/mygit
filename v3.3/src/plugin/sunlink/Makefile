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

BINDIR = $(SWHOME)/bin
PLGDIR = $(SWHOME)/plugin

USERLIB = -lswpack  -lswlsn -lswbase -ldl
SUNLIBS = -lcommkey -lSDKcomm -lSDKdb -lSDKi18n -lSDKipc -lSDKpub -lSDKsecu -lSDKxml -lz -lcrypto -lMYJSDK

OBJS1 = suncli.o
OBJS2 = sunsvr.o
OBJS3 = sunsc.o
OBJS4 = sunss.o

TARGET1 = suncli
TARGET2 = sunsvr
TARGET3 = sunsc.so
TARGET4 = sunss.so

TARGETS = $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)

all:   $(TARGETS) clean
$(TARGET1):$(OBJS1)
	$(CC) $(CFLAGS) -o $@ $(OBJS1) $(APPLIBDIR) $(USERLIB) -L$(SUNLINKHOME)/lib $(SUNLIBS)
	@rm -f $(BINDIR)/$@
	@rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	echo "$@ is ok!"

$(TARGET2):$(OBJS2)
	$(CC) $(CFLAGS) -o $@ $(OBJS2) $(APPLIBDIR) $(USERLIB) -L$(SUNLINKHOME)/lib $(SUNLIBS)
	@rm -f $(BINDIR)/$@
	@rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	echo "$@ is ok!"

$(TARGET3):$(OBJS3)
	$(CC) $(GFLAGS) -o $@ $(OBJS3) $(APPLIBDIR) $(USERLIB) -L$(SUNLINKHOME)/lib $(SUNLIBS) -L/usr/lib64
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(TARGET4):$(OBJS4)
	$(CC) $(GFLAGS) -o $@ $(OBJS4) $(APPLIBDIR) $(USERLIB) -L$(SUNLINKHOME)/lib $(SUNLIBS) -L/usr/lib64
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) -c $*.c -I$(SUNLINKHOME)/sunlink/include

clean:
	rm -f *.*o

