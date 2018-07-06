#!/bin/sh
#set -x
# function  根据流水号获取业务日志并对日志做规整处理

if [ $# -ne 3 ];then
	echo  "err Use: get_log.sh source trcno flag!"
	exit -1
fi

trcno=$2
flag=$3
iflag=1
coun=0

if [ -z "$SWWORK" ];then
	echo "err ".__FILE__." ".__LINE__." No env SWWORK!"
	exit -1
fi

log_path="$SWWORK/dat/log"
if [ ! -d "$log_path" ];then
	mkdir $log_path
fi

time_val=`date +%s`
filename=$log_path/$time_val
touch $filename

if [ $flag = 0 ];then
	log_file=`ls $1_*.log`

	awk /^$trcno/ $1_*.log|grep $trcno >> $filename
	if [ $? -ne 1 ]
	then
		iflag=0
	fi
elif [ $flag -eq 1 ];then
	awk /^$trcno/ $1.pkg*|grep $trcno | awk -F \| '{print $2}' >> $filename
	if [ $? -ne 1 ]
	then
		iflag=0
	fi
#	for line in $pkg
#	do
#		iflag=0
#		
#		if [ "$(echo $line|awk '{if($0~/#/) print 1}')" = "1" ];then
#			tmp=${line#*[}
#			buf=${tmp%%]*}
#			printf "%-16s" $line >> $filename
#		elif [ "$(echo $line|awk '{if($0~/=/) print 1}')" = "1" ];then
#			printf " %s" $line >> $filename
#		elif [ "$(echo $line|awk '{if($0~/* /) print 1}')" = "1" ];then
#			printf " %s" $line >> $filename
#		else
#			printf " %s" $line >> $filename
#			printf "\n" >> $filename
#		fi
#
#	done
else
	echo "err 获取业务日志类别错误!"
	exit -1
fi
	
if [ $iflag -eq 1 ];then
	echo "err 没有流水号 : [$trcno] !"
	exit -1
fi
echo "name:$filename"
