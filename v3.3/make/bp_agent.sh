#!/bin/sh -f
if [ $# -ne 1 ];then
	echo "err:input parameter errors,use the sample:bp_agent.sh --v=3.0.0"
	exit 0
fi

version=$1
cd $SWHOME
tar -cvf agent.tar lib/libstatgrab.a* sbin src/agent >/dev/null 2>&1
gzip agent.tar

FILE_SIZE=`wc -c agent.tar.gz|awk '{print $1}'`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
VERS_LINE_SIZE=`echo "VERSION=$version" | wc -c`
cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_agent.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $VERS_LINE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`
echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "VERSION=$version" >> tmp.a
cat ori_agent.sh >> tmp.a
mv tmp.a $SWHOME/agent.sh
cd ..
cat agent.sh agent.tar.gz > agent.bin
rm -rf agent.sh agent.tar.gz
chmod +x agent.bin

echo "info: release over, agent.bin on $SWHOME"
 
