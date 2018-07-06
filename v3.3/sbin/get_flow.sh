#!/bin/sh
#set -x
#function : 从平台业务日志中获取步骤流程

if [ $# -ne 2 ]
then
	echo "err: Use: get_flow.sh filename trno!"
	exit -1
fi

trcno=$2

log_file=`ls $1_*.log`

awk /^$trcno/ $1_*.log|grep -e 步骤 -e 原子交易异常结束 |awk '{if($0~/原子交易异常结束/) {print "步骤 R"} else {print $2}}'
if [ $? -ne 0 ]
then
	echo "err: 没有流水号 : $trcno !"
	exit -1
fi
