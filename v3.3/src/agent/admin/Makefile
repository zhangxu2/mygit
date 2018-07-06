#------------------------------------------------------------------------------#
# 版 本 号 : V1.0
# 作    者 : sunlian duke zhanghailu liteng
# 日    期 : 2011-10-12
# 所属模块 : 侦听管理模块可执行源码
# 名    称 : lsnman
# 作    用 :
# 使用注意 :
# 修改日志 :
#     修改作者 :
#     修改时间 :
#     修改内容 :
#------------------------------------------------------------------------------#
include $(SWHOME)/make/agtenv

USERLIB = -losbase -lswbase -ldl -lagent_pub 

TARGET = swagent
OBJS = agent_adm.o

all: $(TARGET) clean
$(TARGET): $(OBJS)
	$(CC) $(LFLAGS)  -o  $@ $(OBJS) $(APPLIBDIR) $(USERLIB)
	@rm -f $(BINDIR)/$@
	@rm -f $(BINDIR)/$@.$(VERSION)
	mv $@ $(BINDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(BINDIR)/$@
	@rm -f *.o
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS) $(APPINCLUDE)  -I../include/ -c $*.c
clean:
	rm -f *.o
