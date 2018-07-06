#!/bin/sh -f
#function: update third plugin lib

echo "please input update version:"
echo "eg: 3.0.1"
read version

echo "please input update packno:"
echo "eg: pack001"
read packno

echo "please input update system number:"
echo "eg: 64/32"
read bits
if [ "$bits" = "64" ]
then
	SYSTYPE=" -D__SYSTYPE_64__ "
fi
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
echo "eg: tuxlc.so.3.0.1  libswora.so.3.0.2"
read updfile

#echo "please input update file size:"
#echo "warning: input sequence with update order file"
#echo "eg:30546,123424"
#read updsize

#for libs in $updfile
#do
#	tmp=`ls $SWHOME/lib/$libs 2>/dev/null`
#	stmp=`ls $SWHOME/plugin/$libs 2>/dev/null`
#	if [ -z "$tmp" -a -z "$stmp" ];then
#		echo "input update file not exist, please check!"
#		exit -1
#	fi
#done	

echo "please input update reason:"
echo "eg: modify pkg deal"
read info

datetime=`date +%Y%m%d`
#由于第三方库是在现场编译，不知道库大小，则初始为null,
updsize="null"
#s=`echo $updfile| tr " " "|"`
s=`echo $updfile| tr " " ","`
t=`echo $info| tr " " "-"`

upd_info=$packno"|"$version"|"$datetime"|"$updsize"|"$s"|"$t
#echo $upd_info
#upd_info=`echo "$version $datetime $s $t" | awk '{printf "%-7s   %-8s   %-52s   %-32s", $1,$2,$3,$4}'`

third()
{
cat << END
________________________________________________________________________________________________________
****************please input you want build third party*************************
		  a.mqcom  b.tea  c.tlq  d.tux  e.cics f.ora g.ifx  h.sunlink i.tuxf j.teaf k.jmq l.mqc m.db2
________________________________________________________________________________________________________
END
}

third
read answer
srcpath=$SWHOME/src/plugin
if [ ! -d "$srcpath" ]
then
	echo "$SWHOME/src/plugin not exist!"
	exit 1
fi

echo "answer=$answer"
dates=`date +%Y%m%d`
path="$SWHOME/src/$dates"
echo "path========$path"
if [ ! -d $path ];then
	mkdir $path
else
	mv $path $path_"bak"
	mkdir $path
fi

cd $SWHOME/src/plugin
for var in $answer
do
		case $var in
		a|A)
			mqcombak="mqcom_bak"
			cp -rf mq/com $mqcombak
		;;
		b|B)
			teabak="tea_bak"
			cp -rf tea $teabak
		;;
		c|C)
			tlqbak="tlq_bak"
			cp -rf tlq $tlqbak
		;;
		d|D)
			tuxbak="tux_bak"
			cp -rf tux $tuxbak
		;;
		e|E)
			cicsbak="cics_bak"
			cp -rf cics $cicsbak
		;;
		f|F)
			orabak="ora_bak"
			cd $SWHOME/src/udbc
			cp -rf ora $orabak
			cd $SWHOME/src/plugin
		;;
		g|G)
			ifxbak="ifx_bak"
			cd $SWHOME/src/udbc
			cp -rf ifx $ifxbak
			cd $SWHOME/src/plugin
		;;
		h|H)
			sunbak="sun_bak"
			cp -rf sunlink $sunbak
		;;
		i|I)
			tuxfbak="tuxf_bak"
			cd $SWHOME/src/plugin/file
			cp -rf tuxf $tuxfbak
			cd $SWHOME/src/plugin 
		;;
		j|J)
			teafbak="teaf_bak"
			cd $SWHOME/src/plugin/file
			cp -rf teaf $teafbak
			cd $SWHOME/src/plugin
		;;
		k|K)
			jmqbak="jmq_bak"
			cp -rf mq/jmq $jmqbak
#			cp -f mq/com/mq_pub.h $jmqbak
		;;
		l|L)
			mqcbak="mqc_bak"
			cp -rf mq/mqc $mqcbak
#			cp -f mq/com/mq_pub.h $mqcbak
		;;
		m|M)
			db2bak="db2_bak"
			cd $SWHOME/src/udbc
			cp -rf odbc $db2bak
			cd $SWHOME/src/plugin
		;;
		esac		
done

if [ ! -f "$SWHOME/bin/swmake" ];then
	echo "swmake not exist, please checkout build swmake!"
	exit 0
fi
bakdir="$mqcombak $teabak $tlqbak $tuxbak $cicsbak $sunbak $mqcbak $jmqbak"
list=`echo $bakdir | sed s/[[:space:]]//g`
if [ -n "$list" ];then
	for dir in $bakdir
	do
		cd $dir
		FILES=`ls`
		if [ -z "$FILES" ];then
			continue
		else
				for FILE in $FILES
				do
					swmake all $FILE
				done
				cd ..
		fi	
	done
	
	tar cvf tmp.tar $bakdir >/dev/null 2>&1
	rm -rf $bakdir
	mv tmp.tar $path
fi
		
dbdir="$orabak $ifxbak $db2bak"
list=`echo $dbdir | sed s/[[:space:]]//g`
if [ -n "$list" ];then
	cd $SWHOME/src/udbc
	for dir in $dbdir
	do
		cd $dir
		FILES=`ls`
		if [ -z "$FILES" ];then
					continue
		else
			for FILE in $FILES
			do
				swmake all $FILE
			done
			cd ..
		fi
	done		
	
	tar -cvf udbc.tar $dbdir >/dev/null 2>&1
	rm -rf $dbdir
	mv udbc.tar $path
fi

filedir="$tuxfbak $teafbak"
list=`echo $filedir | sed s/[[:space:]]//g`
if [ -n "$list" ]
then
	cd $SWHOME/src/plugin/file
	for dir in $filedir
	do
		cd $dir
		FILES=`ls`
		if [ -z "$FILES" ];then
			continue
		else
			for FILE in $FILES
			do
				swmake all $FILE
			done
			cd ..
		fi
	done
	
	tar -cvf file.tar $filedir >/dev/null 2>&1
	rm -rf $filedir
	mv file.tar $path
fi

cp -Rf $SWHOME/src/incl_pub $path				
cd $path
tar -cf plugin.tar *
gzip plugin.tar
rm -f plugin.tar 
mv plugin.tar.gz $SWHOME
cd $SWHOME
rm -rf $path

cd $SWHOME
FILE_SIZE=`wc -c plugin.tar.gz|awk '{print $1}'`
HEAD_LINE_SIZE=`echo '#!/bin/sh'|wc -c`
FILE_LINE_SIZE=`echo "FILE_SIZE=$FILE_SIZE"|wc -c`
VERS_LINE_SIZE=`echo "VERSION=$version" | wc -c`
UPDFILE_SIZE=`echo "updfile='$updfile'" | wc -c`
UPDINFO_SIZE=`echo "upd_info='$upd_info'" | wc -c`
SYSTYPE_SIZE=`echo "SYSTYPE='$SYSTYPE'" | wc -c`

cd $SWHOME/make
SHELL_ORI_SIZE=`wc -c ori_upd_plg.sh|awk '{print $1}'`
ALL_SIZE=`expr $FILE_LINE_SIZE + $HEAD_LINE_SIZE + $SHELL_ORI_SIZE + $VERS_LINE_SIZE + $UPDFILE_SIZE + $UPDINFO_SIZE + $SYSTYPE_SIZE`
SIZE=`echo "SHELL_SIZE=$ALL_SIZE"|wc -c`
SHELL_SIZE=`expr $SIZE + $ALL_SIZE`

echo '#!/bin/sh' > tmp.a
echo "SHELL_SIZE=$SHELL_SIZE" >> tmp.a
echo "FILE_SIZE=$FILE_SIZE" >> tmp.a
echo "VERSION=$version" >> tmp.a
echo "updfile='$updfile'" >> tmp.a
echo "upd_info='$upd_info'" >> tmp.a
echo "SYSTYPE='$SYSTYPE'" >> tmp.a
cat ori_upd_plg.sh >> tmp.a
mv tmp.a $SWHOME/updateplugin.sh
cd ..
cat updateplugin.sh plugin.tar.gz > updplg.bin
rm -rf updateplugin.sh plugin.tar.gz
chmod +x updplg.bin
