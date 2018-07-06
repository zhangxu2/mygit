#!/bin/sh -f

sel_list()
{
cat << END
---------------------------------------------------------------
agent/comm	agent/main	agent/pol
agent/admin	agent/tx
---------------------------------------------------------------
END
}

echo "please input update version:"
echo "eg: 2.0.1"
read version

echo "input dir in list:"
sel_list
read params

echo "please input update file:"
echo "eg: flw_sp2001.so"
read updfile

if [ -z "$params" -o -z "$updfile" ];then
	echo "input params list or updfile is null"
	exit 1
fi

updatefile=`echo $updfile | sed 's/\.so/\.c/g'`

date=`date +%Y%m%d`
if [  -d "$SWHOME/$date" ];then
	mv $SWHOME/$date $SWHOME/$date_"bak"
fi

mkdir $SWHOME/$date

cd $SWHOME/$date
cp -Rf $SWHOME/src/agent/include ./
for dir in $params
do
	curpath=$SWHOME/src/$dir
	if [ -d $curpath ];then
		if [ "$dir" != "agent/tx" ];then
			cp -Rf $curpath ./
		else
			mkdir tx
			cd $curpath
			cp -f Makefile* $updatefile $SWHOME/$date/tx/
			cd $SWHOME/$date
		fi
	fi

done

tar -cvf oriagent.tar ./*
mv oriagent.tar $SWHOME/
cd $SWHOME
rm -rf $SWHOME/$date
gzip oriagent.tar  2>/dev/null
rm -f oriagent.tar 2>/dev/null

FILE_SIZE=`wc -c oriagent.tar.gz|awk '{print $1}'`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
UPD_INFO_SIZE=`echo "version=$version" | wc -c`
UPD_FILE_SIZE=`echo "updfile='$updfile'" | wc -c`

cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_ori_agent.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $UPD_INFO_SIZE + $UPD_FILE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`

echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "version=$version" >> tmp.a
echo "updfile='$updfile'" >> tmp.a
cat ori_ori_agent.sh >> tmp.a
mv tmp.a $SWHOME/oriagent.sh
cd $SWHOME
cat oriagent.sh oriagent.tar.gz > oriagent.bin
rm -rf oriagent.sh oriagent.tar.gz
chmod +x oriagent.bin
echo "deploy ok..."
