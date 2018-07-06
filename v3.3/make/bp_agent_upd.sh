#!/bin/sh -f

echo "please input update file:"
echo "eg: flw_sp2001.so"
read updfile

echo "please input update reason:"
echo "eg: modify pkg deal"
read info

echo "updfile=$updfile"
date=`date +%Y%m%d`
if [ ! -d "$SWHOME/$date" ];then
	mkdir $SWHOME/$date
else
	mv $SWHOME/$date $SWHOME/$date_"bak"
fi

cd $SWHOME
cp -Rf $SWHOME/src/agent/tx $SWHOME/$date
cp -Rf $SWHOME/src/agent/include $SWHOME/$date
cd $SWHOME/$date
tar -cvf oriagent.tar ./*
mv oriagent.tar $SWHOME/
cd $SWHOME
rm -rf $SWHOME/$date

cd $SWHOME
gzip oriagent.tar  2>/dev/null
rm -f oriagent.tar 2>/dev/null

updsize="null"
s="oriagent"
t=`echo $info| tr " " "-"`
upd_info=$packno"|"$version"|"$date"|"$updsize"|"$s"|"$t

FILE_SIZE=`wc -c oriagent.tar.gz|awk '{print $1}'`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
UPD_INFO_SIZE=`echo "upd_info='$upd_info'" | wc -c`
UPD_FILE_SIZE=`echo "updfile='$updfile'" | wc -c`

cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_agent_upd.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $UPD_INFO_SIZE + $UPD_FILE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`

echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "upd_info='$upd_info'" >> tmp.a
echo "updfile='$updfile'" >> tmp.a
cat ori_agent_upd.sh >> tmp.a
mv tmp.a $SWHOME/oriagent.sh
cd $SWHOME
cat oriagent.sh oriagent.tar.gz > oriagent.bin
rm -rf oriagent.sh oriagent.tar.gz
chmod +x oriagent.bin
