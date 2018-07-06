#!/bin/sh

if [ $# -ne 1 ];then
	echo "usage: upd_list.sh pack001"
	exit -1
fi

packinfo=$1

pack()
{
cat << END
patches info: [$packinfo]
----------------------------------------------------------------------------------
Rely Version: $revers
Release time: $retime
Install time: $intime
Update  size: $updsize
Update  file: $updlib
Update  info: $updinfo
----------------------------------------------------------------------------------
END
}

cat $SWHOME/etc/.updlist | while read line
do
	if [ "$(echo $line | awk '{if($0~/'"$packinfo"'/) print 1}')" = "1" ];then
		n=`echo "$line" | awk '{print split($0, item, "|")}'`
		i=0
		j=0
		while [ $i -lt $n ]
		do
		        i=`expr $i + 1`
		        buf=`echo $line | cut -d \| -f $i`
		        item[$j]=$buf
		        j=`expr $j + 1`
		done
		
		revers=${item[1]}
		retime=${item[2]}
		intime=${item[6]}
		updsize=${item[3]}
		updlib=${item[4]}
		updinfo=${item[5]}
		pack
		break
	fi
done


