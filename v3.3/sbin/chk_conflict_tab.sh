#!/bin/sh

func_ora()
{
	VALUE=`sqlplus -S "$user/$passwd" <<END
	set pagesize 0 feedback off verify off heading off echo off
	$1
	exit;
      	END`
      	if [ -z "$VALUE" ]
      	then
		continue
      	else
		echo $VALUE >> $2
      	fi
}
fun_inf()
{
	if [ $# -eq 1 ]
	then
		sets="output to pipe cat without headings"
	elif [ $# -eq 2 ]
	then
		sets="unload to $2 delimiter '|'"
	fi

	dbaccess qhrcb_ncbs << !
	$sets
	$1
!
}
func_dbs()
{
	type=`cat $file | grep "DB_TYPE" | awk -F "=" '{print $2}'`
	if [ "$type" = "oracle" ]
	then
		func_opt="func_ora"
	fi
	if [ "$type" = "informix" ]
	then
		func_opt="func_inf"
	fi
	$func_opt "$1" "$2"
}
function connect
{
	swwork=$SWWORK
	file=$swwork"/cfg/develcfg/perl.cfg"
	user=$DB_USER
	if [ -z "$user" ]
	then
		echo "err:[$0][$LINENO]  no env DB_USER!"
		exit -1
	fi
	passwd=$DB_PASSWD
	if [ -z "$passwd" ]
	then
		rm -f $tablename $tabletmp $tablechange
		echo "err:[$0][$LINENO] no env DB_PASSWD!"
		exit -1
	fi

	while read name 
	do
		func_dbs "select table_name from user_tables where table_name = '$name';" $tabletmp
	done < $tablename

}
swhome=$SWHOME
filename=$SWWORK"/tmp/tmpfile"
cat $1 | tr a-z A-Z > $filename
outinfo=$2
tablename=$SWWORK"/tmp/table.name"
tablechange=$SWWORK"/tmp/table.change"
tabletmp=$SWWORK"/tmp/table_tmp"
cat $filename| grep "DIC"| awk -F '"' '{print $2}'|tr a-z A-Z > $tablename
if [ $? -ne 0 ]
then
	rm -f $tablename
	echo "ok:no table"
	exit 0
fi
connect
if [  -s "$tabletmp" ]
then 
	echo "<Conflicts>" > "$outinfo"
	while read name
	do
		while read line
		do
        		echo $line | grep "$name" >> $tablechange
		done < $filename
	done < $tabletmp
	while read line
	do
        	name=`echo $line | awk -F '"' '{print $2}'` 
        	type=`echo $line | awk -F '"' '{print $6}'`
        	attr=`echo $line | awk -F '"' '{print $4}'` 
        	echo "  <Conflict table=\"$name\" type=\"$type\" attr=\"$attr\" />" >> "$outinfo"
	done < $tablechange
	echo "</Conflicts>" >> "$outinfo"
	rm -f $tablename $tabletmp $tablechange
	echo "err:[$0][$LINENO]table exist!!"
	exit -1
fi
rm -f $tablename 
echo "ok: heck conflict table finished!"
exit 0
