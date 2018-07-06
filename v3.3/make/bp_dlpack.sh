#!/bin/sh -f

if [ $# -ne 2 ];then
	echo "err:input parameter errors,use the sample:bp_dlpack.sh --b=64 --v=3.0.0"
	exit 0
fi

if [ -z "$SWHOME" ];then
	echo "err:SWHOME not exist, please set!"
	exit 0
fi

bits=$1
versions=$2
find $SWHOME/src -name *\.o -exec rm -f {} \;
find $SWHOME/src -name *\.so -exec rm -f {} \;
find $SWHOME/lib -name *\.so* -exec rm -f {} \;
find $SWHOME/bin -name *\.[0-9]\.[0-9]\.[0-9] -exec rm -f {} \;
find $SWHOME/plugin -name *\.so* -exec rm -f {} \;

cd $SWHOME/make
. $SWHOME/make/configure $bits $versions
. $SWHOME/make/swbuild nd no
cat $SWHOME/make/.dirname
. $SWHOME/make/.dirname

for DIR in $DIRLIST
do
	case $DIR in
		ORACLE_HOME)
			cd $SWHOME/src/udbc/ora
			make clean
			make
		;;
		INFORMIXDIR)
			cd $SWHOME/src/udbc/ifx
			make clean
			make
		;;
		DB2_HOME)
			cd $SWHOME/src/udbc/odbc
			make clean
			make
		;;
		TUXDIR)
			cd $SWHOME/src/plugin/tux
			make clean
			make

			cd $SWHOME/src/plugin/file/tuxf
			make clean
			make
		;;
		MQDIR)
			cd $SWHOME/src/plugin/mq/com
			make clean
			make
			cd $SWHOME/src/plugin/mq/mqc
			make clean
			make
			echo "Do you want use jmq,[Y/N]?"
			read yy
			if [ "$yy" = "Y" -o "$YY" = "y" ];then
				cd $SWHOME/src/plugin/mq/jmq
				make clean
				make
			fi
		;;
		TEADIR)
			cd $SWHOME/src/plugin/tea
			make clean
			make
			
			cd $SWHOME/src/plugin/file/teaf
			make clean
			make
		;;
		TLQDIR)
			cd $SWHOME/src/plugin/tlq
			make clean
			make
		;;

		SUNDIR)
			cd $SWHOME/src/plugin/sunlink
			make clean
			make
		;;
	esac
done

PLUGIN=$SWHOME/src/plugin
PLUGINBAK=$SWHOME/src/plugin_bak
SRCDIR=$SWHOME/src
DBDIR=$SWHOME/src/udbc
DBBAK=$SWHOME/src/udbc_bak

if [ ! -f "$SWHOME/bin/swmake" ];then
	echo "err:swmake not exist, please checkout build swmake!"
	exit 1
fi

SRCLIST="
mq
tea
tlq
tux
sunlink
file/tuxf
file/teaf
cics"

cd $SWHOME
cp -rf $PLUGIN $PLUGINBAK

for DIR in $SRCLIST
do	
	if [ -d "$PLUGINBAK/$DIR" -a "$DIR" != "mq" ];then
		cd $PLUGINBAK/$DIR
	else
		continue
	fi
	for FILE in `ls`
	do
		$SWHOME/bin/swmake all $FILE
	done
	cd $PLUGINBAK 
done

tar -cvf plugin.tar mq tea tlq tux sunlink file/tuxf file/teaf cics>/dev/null 2>&1
cd ..
mv $PLUGINBAK/plugin.tar ./
rm -rf $PLUGINBAK

DBLIST="
ora
ifx
odbc
"

cp -rf $DBDIR  $DBBAK
for dir in $DBLIST
do	
	if [ -d "$DBBAK/$dir" ];then
		cd $DBBAK/$dir
	else
		continue
	fi
	
	for FILE in `ls`
	do
		$SWHOME/bin/swmake all $FILE
	done
	cd ..
done

tar -cvf udbc.tar ora ifx odbc >/dev/null 2>&1
cd ..
mv $DBBAK/udbc.tar ./
rm -rf $DBBAK

tar -cvf src.tar incl_pub plugin.tar udbc.tar >/dev/null 2>&1
rm -f plugin.tar udbc.tar
cd ..
mv $SRCDIR/src.tar ./
tar -cvf bp.tar bin lib plugin etc make include sample src.tar >/dev/null 2>&1
rm -f src.tar
gzip bp.tar
mv bp.tar.gz $SWHOME/make
cd  $SWHOME/make

FILE_SIZE=`wc -c bp.tar.gz|awk '{print $1}'`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
SHELL_ORI_SIZE=`wc -c ori_dldeploy.sh|awk '{print $1}'`
BIT_LINE_SIZE=`echo "bit=$bits" | wc -c`
VERS_LINE_SIZE=`echo "version=$versions" | wc -c`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $BIT_LINE_SIZE + $VERS_LINE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`
echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "bit=$bits" >> tmp.a
echo "version=$versions" >> tmp.a
cat ori_dldeploy.sh >> tmp.a
mv tmp.a bp_dldeploy.sh
cat bp_dldeploy.sh bp.tar.gz > DFIS-BP.bin
rm -f bp_dldeploy.sh bp.tar.gz
mv DFIS-BP.bin $SWHOME
echo "info: release over,DFIS-BP.bin on $SWHOME!"
