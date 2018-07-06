#------------------------------------------------------------------------------#
# 版 本 号 : V1.0
# 作    者 : MaWeiwei YeMeng ZhaoDazeng
# 日    期 : 2012-02-10
# 所属模块 :
# 名    称 :
# 作    用 :
# 使用注意 :
# 修改日志 :
# 修改作者 :
# 修改时间 :
# 修改内容 :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env
USRLIB=-lpthread -lswbase -lssl -lcrypto

TARGETS = liballinpaysecure.so
OBJS = allinpaysecure.o

all: $(TARGETS)
$(TARGETS): $(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(USRLIB) 
	rm -f $(PLGDIR)/$@
	mv $@ $(PLGDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -I/home/zfpt/usr/mww/openssl-1.0.1g/include -c $*.c

clean:
	rm -f *.o *.so
