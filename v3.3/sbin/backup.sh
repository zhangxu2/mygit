#!/bin/sh

function cp_bak()
{
	if [ $# -ne 2 ]
	then
		echo "err:parameter num err!!"
		exit -1
	fi
	path=$1
	target=$2
	file= 
	date=`date +%Y%m%d`
	
	if [ ! -d "$path" ]
	then
		echo "err:No dir $parh!!"
		exit -1
	fi
	echo $path | grep "dat" > /dev/null
	if [ $? -eq 0 ]
	then
		tmp=$target/dat_$date
		dif=dat_$date
	fi
	echo $path | grep "monitor" > /dev/null
	if [ $? -eq 0 ]
	then
		tmp=$target/monitor_$date
		dif=monitor_$date
	fi
	for line in `ls $path`
	do
                file="$file `echo $line | grep $date`"
                if [ -z "$file" ]
                then
                	continue
                fi
        done

	cd $path
        tar zcvf $dif.tar.gz $file > /dev/null
        if [ $? -ne 0 ]
        then
                echo "[$0][$LINE]tar dat_$date err!"
                exit -1
        fi
        mv $dif.tar.gz $target
        #rm -rf $file 
}


Time=`date +%H:%M:%S`
date=`date +%Y%m%d`
B_time="01:00:00"
target=$HOME/usr/hgg/bak
if [ ! -d $target ]
then
	mkdir -p $target
fi

dat=$SWWORK/dat
monitor=$SWWORK/tmp/monitor
#cp_bak $dat  $target
#cp_bak $monitor  $target

