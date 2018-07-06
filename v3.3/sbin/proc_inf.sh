#!/bin/sh

#  name : proc_if.sh
# function : 获取最耗费CPU 内存的10个进程信息

if [ $# -ne 1 ];then
	printf "err: Use: proc_inf.sh CPU/MEM!\n"
	exit -1
fi

mod=$1

if [ "$mod" != "CPU" -a "$mod" != "MEM" ];then
	printf "err: Unknown mod\[$mod\]!\n"
	exit -1
fi

ret=`uname`
os=`echo ${ret[0]} | tr '[A-Z]' '[a-z]'`

if [ "$os" = "linux" ];then
if [ "$mod" = "CPU" ];then
	ps -eo user,pid,pcpu,thcount,pmem,vsz,etime,stat,comm|sort -rn -k +3|head -20
else
	ps -eo user,pid,pcpu,thcount,pmem,vsz,etime,stat,comm|sort -rn -k +5|head -20
fi
fi

if [ "$os" = "aix" ];then
if [ "$mod" = "CPU" ];then
        ps -eo user,pid,pcpu,thcount,pmem,vsz,etime,stat,comm|sort -rn +3|head -20
else
        ps -eo user,pid,pcpu,thcount,pmem,vsz,etime,stat,comm|sort -rn +5|head -20
fi
fi



