#!/bin/sh -f

sel_list()
{
cat << END
---------------------------------------------------------------
input params list:
---------------------------------------------------------------
os/unix           core                  pack
uniform           alert			admin 
job		  logs			comm
pol		  udbc/com   	        svc
udbc/ifx	  plugin/allinpaysecure udbc/odbc	
lsn/map           lsn/comm 		lsn/tcp
lsn/svrmap	  udbc/ora		plugin/zip 	  
tools/cipc	  tools/swmake          tools/minfo  
tools/mipc        tools/txview          tools/swreg 
plugin/des        plugin/udf            plugin/autosend       
plugin/dealpkg    plugin/deny           plugin/file/tcpf      
plugin/link       plugin/jzcardpack     plugin/http
plugin/maptxcode  plugin/soap           plugin/start      
---------------------------------------------------------------
end list params
---------------------------------------------------------------
END
}

echo "please input update version:"
echo "eg: 3.1.1"
read version

echo "please input update packno:"
echo "eg: pack001"
read packno

if cat $SWHOME/make/bp_pack_list.sh | grep "$packno"
then
	echo "Input number already exists, please re-entry, numbered list:"
	cat $SWHOME/make/bp_pack_list.sh | while read line
	do
		echo "$line"
	done
	exit 0
else
	echo "$packno" >> $SWHOME/make/bp_pack_list.sh
fi

echo "please input update file:"
echo "eg: tcpsc.so libswlsn.so"
read updfile

echo "please input update reason:"
echo "eg: modify pkg deal"
read info

echo "please input update oridir:"
echo "input dir in list:"
sel_list
read params

DIRLIST="
os/unix
core
pack
uniform
admin
alert
job
pol
comm
logs
udbc/com
udbc/ifx
udbc/ora
udbc/odbc
svc
lsn/map
lsn/comm
lsn/tcp
lsn/svrmap
plugin/des
plugin/udf
plugin/autosend
plugin/dealpkg
plugin/allinpaysecure
plugin/link
plugin/http
plugin/zip
plugin/deny
plugin/jzcardpack
plugin/maptxcode
plugin/packcmt
plugin/savepkg
plugin/soap
plugin/start
plugin/windes
plugin/file/tcpf 
tools/swmake
tools/cipc
tools/minfo
tools/mipc
tools/swreg
"
date=`date +%Y%m%d`
echo "date=$date"
mkdir $SWHOME/$date
inputcnt=`echo $params | awk -F " " '{print NF}'`
flag=0
issep=
for upd in $params
do
	for DIR in $DIRLIST
	do
		if [ "$upd" = "$DIR" ];then
			if [ -d "$SWHOME/src/$DIR" ];then
				cp -Rf $SWHOME/src/$DIR $SWHOME/$date/
				issep=`echo $upd | awk -F '/' '{print $NF}'`
				if [ -n "$issep" ];then
					tmppara="$tmppara $issep"
				else
					tmppara="$tmppara $upd"
				fi
				flag=`expr $flag + 1`
				break;
			fi
		fi
	done
done

params=$tmppara
echo "params=$params"
echo "updfile=$updfile"

cd $SWHOME/$date
tar -cvf oriupd.tar ./*
mv oriupd.tar $SWHOME/
cd $SWHOME
rm -rf $SWHOME/$date
if [ $flag -ne $inputcnt ];then
	echo "already find all dir, not find all input params,please check."
	exit -1
fi

cd $SWHOME
gzip oriupd.tar  2>/dev/null
rm -f oriupd.tar 2>/dev/null

updsize="null"
s="oriupd"
t=`echo $info| tr " " "-"`
upd_info=$packno"|"$version"|"$date"|"$updsize"|"$s"|"$t

FILE_SIZE=`wc -c oriupd.tar.gz|awk '{print $1}'`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
UPD_INFO_SIZE=`echo "upd_info='$upd_info'" | wc -c`
UPD_FILE_SIZE=`echo "updfile='$updfile'" | wc -c`
DIR_INFO_SIZE=`echo "dir_info='$params'" | wc -c`

cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_ori_upd.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $UPD_INFO_SIZE + $UPD_FILE_SIZE + $DIR_INFO_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`

echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "upd_info='$upd_info'" >> tmp.a
echo "updfile='$updfile'" >> tmp.a
echo "dir_info='$params'" >> tmp.a
cat ori_ori_upd.sh >> tmp.a
mv tmp.a $SWHOME/oriupd.sh
cd $SWHOME
cat oriupd.sh oriupd.tar.gz > oriupd.bin
rm -rf oriupd.sh oriupd.tar.gz
chmod +x oriupd.bin
