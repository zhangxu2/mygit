#!/bin/sh
#name:bp_upd_lib.sh
#function:update lib file

echo "please input update version:"
echo "eg: 3.0.1"
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
echo "eg: libswpack.so.3.0.1  tcpss.so.3.0.1"
read name
echo "please input update file size:"
echo "warning: input sequence with update order file"
echo "eg:30546,123424"
read updsize

for libs in $name
do
	tmp=`ls $SWHOME/lib/$libs 2>/dev/null`
	stmp=`ls $SWHOME/plugin/$libs 2>/dev/null`
	if [ -z "$tmp" -a -z "$stmp" ];then
		echo "input update file not exist, please check!"
		exit -1
	fi
done	

echo "please input update reason:"
echo "eg: modify pkg deal"
read info

datetime=`date +%Y%m%d`

dirlist="lib plugin"

for binfile in $name
do
	for list in $dirlist
	do
		flag=0
		cd $SWHOME/$list
		for base in `ls`
		do
			if [ "$base" = "$binfile" ];then
				cp $binfile $SWHOME
				flag=1
				break
			fi
			flag=0
		done
		if [ "$flag" -eq 1 ];then
			break
		fi
	done
done

cd $SWHOME

tar -cvf lib.tar $name 2>/dev/null
gzip lib.tar  2> /dev/null
rm -f lib.tar $name

#s=`echo $name| tr " " "|"`
s=`echo $name | tr " " ","`
t=`echo $info| tr " " "-"`

upd_info=$packno"|"$version"|"$datetime"|"$updsize"|"$s"|"$t
#upd_info=`echo "$version $datetime $s $t" | awk '{printf "%-7s   %-8s   %-52s   %-32s", $1,$2,$3,$4}'`
FILE_SIZE=`wc -c lib.tar.gz|awk '{print $1}'`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
UPD_INFO_SIZE=`echo "upd_info='$upd_info'" | wc -c`

cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_upd_lib.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $UPD_INFO_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`

echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "upd_info='$upd_info'" >> tmp.a
cat ori_upd_lib.sh >> tmp.a
mv tmp.a $SWHOME/updlib.sh
cd ..
cat updlib.sh lib.tar.gz > updlib.bin
rm -rf updlib.sh lib.tar.gz
chmod +x updlib.bin
