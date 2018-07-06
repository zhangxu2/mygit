#!/bin/sh
# function : ��ҵ����־�л�ȡ���ܽڵ�����
#set -x

if [ $# -ne 2 ]
then
	echo "err: Use: get_flow.sh filename flow!"
	exit -1
fi

trcno=$2
iflag=1

myline=`awk /^$trcno/ $1_*.log|grep -e ���� -e �ڵ� -e ԭ�ӽ��� | awk '{print $2}'`
for line in $myline
do
		if [ ! -z "$(echo $line|grep ����)" ]
		then
			tmp=`echo $line | awk -F ":" '{print $NF}'`
			echo "$tmp"
		elif [ ! -z "$(echo $line|grep �ڵ�)" ]
		then	
			tmp=`echo $line | awk -F " " '{print $NF}'| sed 's/\(I.\)//g'`
			echo "$tmp"
		elif [ ! -z "$(echo $line|grep ԭ�ӽ���)" ]
		then
			if [ ! -z "$(echo $line|grep ԭ�ӽ����쳣����)" ]
			then
				echo "ԭ�ӽ���:R"
			else
				echo "ԭ�ӽ���:${line##*ԭ�ӽ���}"
			fi
		fi
done

if [ -z "$myline" ];then
	echo "err: û����ˮ�� : $trcno !"
	exit -1
fi

