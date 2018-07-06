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

PROC = proc
PROCINCLUDE = include=$(ORACLE_HOME)/precomp/public  include=$(SWHOME)/src/incl_pub include=.

USRLIB= -lswbase -ldl
ORALIB  = -lclntsh -lpthread -lc `cat $(ORACLE_HOME)/lib/sysliblist`
#ORALIB  = -lclntsh -lpthread -ldl -lc -lm -lpthread -lodm -lbsd_r -lld -lperfstat

INCLUDE_PATH= -I.\
		-I$(ORACLE_HOME)/precomp/public \
		-I$(ORACLE_HOME)/rdbms/public \
		-I$(ORACLE_HOME)/rdbms/demo\
		-I$(SWHOME)/src/incl_pub


LIB_PATH = -L$(ORACLE_HOME)/lib 

OBJS1 = db_ora_opt.o
OBJS2 = db_ora.o
TARGET1 = libswora.so
TARGET2 = libswora_pc.so
TARGETS = $(TARGET1) $(TARGET2)

all: $(TARGETS) clean

$(TARGET1): $(OBJS1)
	$(LD) -o $@ $(OBJS1) $(LIB_PATH) $(APPLIBDIR) $(USRLIB) $(ORALIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv -f $@ $(LIBDIR)/$@.$(VERSION)
	ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"
	
$(TARGET2): $(OBJS2)
	$(LD) -o $@ $(OBJS2) $(LIB_PATH) $(APPLIBDIR) $(USRLIB) $(ORALIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv -f $@ $(LIBDIR)/$@.$(VERSION)
	ln -s $@.$(VERSION) $(LIBDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"
	
.SUFFIXES: .pc .c .cpp .o
.pc.o:                                                                                                                             
	$(PROC) $(PROCINCLUDE) dbms=v7 unsafe_null=yes lines=yes iname=$*.pc                                                       
	$(CC) $(CFLAGS) $(INCLUDE_PATH) $(APPINCLUDE) -c $*.c                                                                                      
	@rm -f $*.c                                                                                                                
	@rm -f $*.lis 
.c.o:
	$(CC)  $(CFLAGS) $(INCLUDE_PATH) $(APPINCLUDE) -c $*.c

clean:
	rm -f *.o *.so *~

