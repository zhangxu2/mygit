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

USERLIB = -losbase -lswpack -lswmap -lswsoap -ldl
LUSERLIB = -lswbase -lswlsn

OBJS1 = soapss.o httpget.o
TARGET1 = soapss.so

OBJS2 = soapsc.o
TARGET2 = soapsc.so

OBJS3 = soapcall.o
TARGET3 = soapcall.so

SOAPOBJS = stdsoap2.o soap_pub.o
SOAPLIB = libswsoap.so

TARGET = $(TARGET1) $(TARGET2) $(TARGET3)
all: $(SOAPLIB) $(TARGET) clean
$(TARGET1): $(OBJS1)
	$(CC) $(GFLAGS) -o $@ $(OBJS1) $(APPLIBDIR) $(USERLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK!"

$(TARGET2): $(OBJS2)
	$(CC) $(GFLAGS) -o $@ $(OBJS2) $(APPLIBDIR) $(USERLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK!"

$(TARGET3): $(OBJS3)
	$(CC) $(GFLAGS) -o $@ $(OBJS3) $(APPLIBDIR) $(USERLIB) -lswlsn
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK!"

$(SOAPLIB):$(SOAPOBJS)
	$(CC) $(GFLAGS) -o $@ $(SOAPOBJS) $(APPLIBDIR) $(LUSERLIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK!"

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE)  -I. -c $*.c

clean:
	@rm -f *.o core 
