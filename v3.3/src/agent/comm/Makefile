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

USERLIB = -losbase  -lswbase -lswpack -lswlsn -ludbc -lpthread -ldl 

TARGETS1 = libomsfile.so
OBJS1 = agent_tcpf.o

TARGETS2 = libagent_pub.so 
OBJS2 =  agent_comm.o agent_proc.o agent_md5.o agent_pub.o agent_search.o

all:  $(TARGETS1)  $(TARGETS2) clean 

$(TARGETS1): $(OBJS1)
	$(CC) $(GFLAGS) -o $@ $(OBJS1)
	@rm -f $(PLGDIR)/$@
	@rm -f $(PLGDIR)/$@.$(VERSION)
	mv $@ $(PLGDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(PLGDIR)/$@
	@echo "$@ is OK !"

$(TARGETS2): $(OBJS2)
	$(CC) $(GFLAGS)  -o  $@ $(OBJS2) $(APPLIBDIR) $(USERLIB) $(COMMLIB)
	@rm -f $(LIBDIR)/$@
	@rm -f $(LIBDIR)/$@.$(VERSION)
	mv $@ $(LIBDIR)/$@.$(VERSION)
	@ln -s $@.$(VERSION) $(LIBDIR)/$@
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -I../include  -c $*.c
clean:
	rm -f *.o *.so
