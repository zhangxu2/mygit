#!/bin/sh
# function : 从业务日志中获取功能节点流程
#set -x

if [ $# -ne 2 ]
then
	echo "err: Use: get_flow.sh filename flow!"
	exit -1
fi

trcno=$2
iflag=1

myline=`awk /^$trcno/ $1_*.log|grep -e 步骤 -e 节点 -e 原子交易 | awk '{print $2}'`
for line in $myline
do
		if [ ! -z "$(echo $line|grep 步骤)" ]
		then
			tmp=`echo $line | awk -F ":" '{print $NF}'`
			echo "$tmp"
		elif [ ! -z "$(echo $line|grep 节点)" ]
		then	
			tmp=`echo $line | awk -F " " '{print $NF}'| sed 's/\(I.\)//g'`
			echo "$tmp"
		elif [ ! -z "$(echo $line|grep 原子交易)" ]
		then
			if [ ! -z "$(echo $line|grep 原子交易异常结束)" ]
			then
				echo "原子交易:R"
			else
				echo "原子交易:${line##*原子交易}"
			fi
		fi
done

if [ -z "$myline" ];then
	echo "err: 没有流水号 : $trcno !"
	exit -1
fi

