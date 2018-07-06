#!/bin/sh -f
if [ $# -ne 1 ];then
	echo "err:input parameter errors,use the sample:bp_pack.sh --v=3.0.0"
#	echo "warning:Please confirm if already fill out the release information in the record!"
	exit 0
fi

find $SWHOME/src -name *\.o -exec rm -f {} \;
find $SWHOME/src -name *\.so -exec rm -f {} \;
find $SWHOME/lib -name *\.so* -exec rm -f {} \;
find $SWHOME/bin -name *\.[0-9]\.[0-9]\.[0-9] -exec rm -f {} \;
find $SWHOME/plugin -name *\.so* -exec rm -f {} \;

version=$1
cd $SWHOME
tar -cvf bp.tar bin etc include make plugin src >/dev/null 2>&1
gzip bp.tar

FILE_SIZE=`wc -c bp.tar.gz|awk '{print $1}'`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
VERS_LINE_SIZE=`echo "VERSION=$version" | wc -c`
cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_sedeploy.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $VERS_LINE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`
echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "VERSION=$version" >> tmp.a
cat ori_sedeploy.sh >> tmp.a
mv tmp.a $SWHOME/sedeploy.sh
cd ..
cat sedeploy.sh bp.tar.gz > bp.bin
rm -rf sedeploy.sh bp.tar.gz
chmod +x bp.bin

echo "info: release over, bp.bin on $SWHOME"
 
