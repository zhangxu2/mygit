include $(SWHOME)/make/agtenv
USERLIB = -lswbase -losbase -lpthread -lswpack -lswlsn -ldl -luni  -ludbc -lagent_pub  

TARGETS = flw_sp2001.so \
	flw_sp2002.so \
	flw_sp2003.so \
	flw_sp2004.so \
	flw_sp2009.so \
	flw_sp2010.so \
	flw_sp2011.so \
	flw_sp2013.so \
	flw_sp2014.so \
	flw_sp2020.so \
	flw_sp2021.so \
	flw_sp2022.so \
	flw_sp2023.so \
	flw_sp2024.so \
	flw_sp2025.so \
	flw_sp2026.so \
	flw_sp2027.so \
	flw_sp2029.so \
	flw_sp2030.so \
	flw_sp2031.so \
	flw_sp2032.so \
	flw_sp2033.so \
	flw_sp2035.so \
	flw_sp2036.so \
	flw_sp2040.so \
	flw_sp2041.so \
	flw_sp2042.so \
	flw_sp2050.so \
	flw_sp2051.so \
	flw_sp2052.so \
	flw_sp2053.so \
	flw_sp2054.so \
	flw_sp2055.so \
	flw_sp2056.so \
	flw_sp2057.so \
	flw_sp2060.so \
	flw_sp2061.so \
	flw_sp2062.so \
	flw_sp2063.so \
	flw_sp2070.so \
	flw_sp2071.so \
	flw_sp2072.so \
	flw_sp2073.so \
	flw_sp2074.so \
	flw_sp2080.so \
	flw_sp2082.so \
	flw_sp2090.so \
	flw_sp2091.so \
	flw_sp2092.so \
	flw_sp2093.so \
	flw_sp2094.so \
	flw_sp2095.so \
	flw_sp2096.so \
	flw_sp2097.so \
	flw_sp2100.so \
	flw_sp2101.so \
	flw_sp2102.so \
	flw_sp2103.so \
	flw_sp2104.so \
	flw_sp2105.so \
	flw_sp2106.so \
	flw_sp2107.so \
	flw_sp2108.so \
	flw_sp2109.so \
	flw_sp2110.so \
	flw_sp2111.so \
	flw_sp2112.so \
	flw_sp2113.so \
	flw_sp2114.so \
	flw_sp2115.so \
	flw_sp2116.so \
	flw_sp2118.so 

all: $(TARGETS) clean

.SUFFIXES: .c .o .so
.c.o:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -I../include -I$(SWHOME)/src/uniform -c $*.c
.o.so:
	$(CC) $(GFLAGS) -o  $*.so $*.o $(APPLIBDIR) $(USERLIB) $(COMMLIB)
	rm -f $(AGTLIBDIR)/$@
	@rm -f $(AGTLIBDIR)/$@.$(VERSION)
	mv $@ $(AGTLIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(AGTLIBDIR)/$@
	rm -f *.o
	@echo "$* is ok!"
clean:
	rm -f *.o *.so 
