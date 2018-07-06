#------------------------------------------------------------------------------#
# 版 本 号 : 
# 作    者 : 
# 日    期 : 2012-09-13
# 所属模块 : 
# 名    称 : 
# 作    用 :
# 使用注意 :
# 修改日志 :
#     修改作者 :
#     修改时间 :
#     修改内容 :
#------------------------------------------------------------------------------#
include $(SWHOME)/make/sw.env

OBJS 	= swmake.o comm.o

TARGET 	= swmake

all: $(TARGET) clean 
$(TARGET): $(OBJS) 
	$(CC) $(LFLAGS) $(APPLIBDIR)  -o $@ $(OBJS)
	rm -f $(BINDIR)/$@
	mv -f $@ $(BINDIR)/$@
	@echo "$@ is OK !"

.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS)  -c $*.c

clean:
	rm -f *.o
