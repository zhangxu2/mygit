#------------------------------------------------------------------------------#
# 版 本 号 : V1.0 
# 作    者 : sunlian duke zhanghailu liteng   
# 日    期 : 2011-10-12       
# 所属模块 : 公共基础库 
# 名    称 : base  
# 作    用 :    
# 使用注意 :         
# 修改日志 :            
#     修改作者 :    
#     修改时间 :   
#     修改内容 :   
#------------------------------------------------------------------------------#

include $(SWHOME)/make/sw.env
USERLIB = -lswbase -losbase
SLIB = test

OBJS = test.o 

all: $(SLIB) clean

$(SLIB): $(OBJS)
	$(CC) $(LFLAGS) $(APPLIBDIR) $(USERLIB) -o $@ $(OBJS)
	rm -f $(LIBDIR)/$(SLIB)
	mv $(SLIB) $(LIBDIR)
	@echo "$@ is OK !"
.SUFFIXES: .c .o
.c.o:
	$(CC)  $(CFLAGS) $(APPINCLUDE) -c $*.c

clean:
	@rm -f *.o core trc.log

