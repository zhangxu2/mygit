#------------------------------------------------------------------------------#
# 版 本 号 : V1.0
# 作    者 : liteng
# 日    期 : 2011-10-12
# 所属模块 : 存储转发守护进程可执行源码
# 名    称 : saf
# 作    用 :
# 使用注意 :
# 修改日志 :
#     修改作者 :
#     修改时间 :
#     修改内容 :
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env

TUSERLIB = -lswbase -losbase -ludbc -ldl -luni

TARGET = swmipc
OBJS = swmipc.o

all:$(TARGET) clean
$(TARGET):$(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(APPLIBDIR) $(TUSERLIB)
	rm -f $(BINDIR)/$@
	mv $@ $(BINDIR)
	@rm -f *.*o
	@echo "$@ is OK !"

.SUFFIXES: .c .o .so
.c.o:
	$(CC)  $(CFLAGS)  $(APPINCLUDE) -c $*.c

clean:
	@rm -f *.*o *.log
