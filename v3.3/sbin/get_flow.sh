#!/bin/sh
#set -x
#function : ��ƽ̨ҵ����־�л�ȡ��������

if [ $# -ne 2 ]
then
	echo "err: Use: get_flow.sh filename trno!"
	exit -1
fi

trcno=$2

log_file=`ls $1_*.log`

awk /^$trcno/ $1_*.log|grep -e ���� -e ԭ�ӽ����쳣���� |awk '{if($0~/ԭ�ӽ����쳣����/) {print "���� R"} else {print $2}}'
if [ $? -ne 0 ]
then
	echo "err: û����ˮ�� : $trcno !"
	exit -1
fi
