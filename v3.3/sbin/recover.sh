#!/bin/sh

if [ $# -ne 1 ]
then
	echo "Use: recover.pl backid"
	exit -1
fi

if [ -z $HOME ]
then
	echo "not set home,please set!"
	exit -1
fi

if [ -z $SWWORK ]
then
	echo "not set swwork, please set!"
	exit -1
fi

back_dir=$HOME/install/backup$1/work
if [ ! -d $back_dir ]
then
	echo "back dir not exit!"
	exit -1
fi

if [ ! -d $SWWORK ]
then
		echo "$SWWORK not exist!"
		exit -1
fi

timestamp=`date +%s`
mv -f $SWWORK $HOME/install/recover$timestamp

cp -R $back_dir $SWWORK

echo "STDERR ok: recover sucessful!"
exit 0
