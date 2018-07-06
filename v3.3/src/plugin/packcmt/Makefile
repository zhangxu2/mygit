#------------------------------------------------------------------------------#
# 版 本 号 : V1.0
# 作    者 : sunlian duke zhanghailu liteng
# 日    期 : 2011-10-12
# 所属模块 : 报文处理公共模块可执行源码
# 名    称 : pkgpub 
# 作    用 :
# 使用注意 :
# 修改日志 :
#     修改作者 :
#     修改时间 :
#     修改内容 :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

USERLIB = -lswbase -lc

SLIB	= libswpackcmt.so
OBJS	= packin.o packout.o packpub.o

all: $(SLIB) clean
$(SLIB): $(OBJS)
	$(CC) $(GFLAGS) -o $@ $(OBJS) -L$(LIBDIR) $(USERLIB) 
	rm -f $(PLGDIR)/$(SLIB)
	mv $(SLIB) $(PLGDIR)
	@echo "$@ is OK !"
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS)  $(APPINCLUDE) -c $*.c
clean:
	rm -f *.o
