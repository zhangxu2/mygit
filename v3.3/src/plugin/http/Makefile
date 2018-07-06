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

PLGDIR = $(SWHOME)/plugin

USERLIB = -lswpack  -lswlsn -lswbase -lswhttp -ldl $(SSLLIB)
LUSERLIB = -lswpack  -lswlsn -lswbase -ldl $(SSLLIB)

SLIB = httpss.so
OBJS = httpss.o

SLIB1 = httpsc.so
OBJS1 = httpsc.o

HTTPOBJ = http_comm.o
HTTPLIB = libswhttp.so

all: $(HTTPLIB) $(SLIB) $(SLIB1) clean
$(SLIB):$(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(SLIB1):$(OBJS1)
	$(CC) $(GFLAGS) -o $@ $(OBJS1) $(APPLIBDIR) $(USERLIB)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK!"

$(HTTPLIB):$(HTTPOBJ)
	$(CC) $(GFLAGS) -o $@ $(HTTPOBJ) $(APPLIBDIR) $(LUSERLIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@echo "$@ is OK!"

.SUFFIXES: .c .o .so
.c.o:
	$(CC) $(CFLAGS) $(APPINCLUDE) $(OPENSSLINC) -c $*.c

clean:
	rm -f *.*o
