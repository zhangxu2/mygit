#!/bin/sh

if [ $# -lt 5 ]
then
echo "error input"
fi

sql=$1
dbtype=$2
filepath=$3
sid=$4
user=$5
passwd=$6
dbname=$7

mkdir -p $SWWORK/tmp/agent

if [ "$filepath" = "NULL" ]
then
filepath=""
fi

func_orc()
{
sqlplus -S $user/$passwd@$sid <<!
set heading off linesize 9999 colsep'|' feedback off pagesize 0 verify off echo off
$sql 
!
}

func_ifx()
{
if [ "$filepath" != "" ]
then
	sets="unload to $filepath delimiter '|'"
else
	sets="output to pipe cat without headings "
fi
dbaccess $dbname@$sid <<!
$sets
$sql
!
}

if [ "$dbtype" == "ORACLE" ]
then
	func_orc
	
elif [ "$dbtype" == "INFORMIX" ]
then
	func_ifx
fi

if [ $? -ne 0 ];then
	exit -1
fi

exit 0
