DIRLIST=
FILE=
if [ ! -d "$SWHOME" ];then
	echo "error:SWHOME path not exist£¬ please checkout!"
	exit 0
fi

thirds()
{
	DIRLIST=
	
	if [ "$2" = "TUXDIR2" -o "$2" = "TUXFILEDIR" ];then
		default="TUXDIR"
	elif [ "$2" = "TEADIR2" -o "$2" = "TEAFILEDIR" ];then
		default="TEADIR"
	elif [ "$2" = "MQCOMDIR" -o "$2" = "MQCDIR" -o "$2" = "JMQDIR" ];then
		default="MQDIR"
	else
		default=$2
	fi
	eval MSG=\${$default}
	eval OLD=\${$default}

cat << END
	you have choiced $1, please input new information
	default $default="$MSG"
	if you want use the default info, please direct enter, else input path!
END

	read NEW
	
	if [ -z "$NEW" ]
	then
		eval $default=$MSG
	else
		eval $default=$NEW
	fi

	eval MSG=\${$default}
	if [ -z "$MSG" ]
	then
		echo "you have choice $1, but $2 is null, your input is invalid"
		eval $default=$OLD
		return 0
	fi 

	if [ ! -d "$MSG" ]
	then 
		echo "you have choice $1, but $MSG is not directory, your input is invalid"
		eval $default=$OLD
		return 0
	fi

	DIRLIST=$2
	eval $2=$MSG
}

check_trd_update()
{
	pathlist="lib plugin"
	
	for plg in $updfile
	do
		flag=0
		for path in $pathlist
		do
			cd $SWHOME/$path
			stmp=`echo $plg | cut -d '.' -f 1,2`
			if [ ! -f "$plg" -a -f "$stmp" ]
			then
				mv $stmp $stmp.`date +%Y%m%d`
				flag=1
			fi
		
			if [ $flag -eq 1 ];then
				break
			fi
		done
	done
}

deal_upd_file()
{
	FILE=
	dir=$1
	case $dir in
	tux_bak)
		m="tux"
		;;
	tuxf_bak)
		m="libfile"
		;;
	mqcom_bak)
		m="libmqpub"
		;;
	mqc_bak)
		m="mq"
		;;
	jmq_bak)
		m="jmq"
		;;
	cics_bak)
		m="cics"
		;;
	tea_bak)
		m="tea"
		;;
	teaf_bak)
		m="libfile"
		;;
	tlq_bak)
		m="tlq"
		;;
	ora_bak)
		m="ora"
		;;
	ifx_bak)
		m="ifx"
		;;
	db2_bak)
		m="odbc"
		;;
	esac

	for upd in $updfile
	do
		s=`echo $upd | cut -d . -f 1,2`
		t=`echo $s | awk '{if($0~/'"$m"'/) print 1}'`
		if [ "$t" = "1" ];then
			FILE=$s
			break
		fi
	done
}

INSTALL()
{
	echo "now ready to update file, please wait ....."
	dd if="$0" of="$SWHOME/plugin.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1

	cd $SWHOME
	gzip -d plugin.tar.gz 
	if [ ! -f "$SWHOME/bin/swmake" ];then
		echo "error:swmake not exist, please checkout build swmake!"
		rm -rf $SWHOME/plugin.tar
		exit 0
	fi
	
	if [ -d "src" ];then
		mv src src.`date +%Y%m%d`
	fi
			
	mkdir src
	mv plugin.tar src 
	cd src
	tar xf plugin.tar > /dev/null 2>&1
	rm -rf plugin.tar 
	
	if [ -f "$SWHOME/src/tmp.tar" ];then
		mkdir plugin
		mv tmp.tar plugin/
		cd plugin
		tar -xvf tmp.tar > /dev/null 2>&1
		rm -rf tmp.tar 
	
		DIRS=`ls`
		for DIR in $DIRS
		do
			cd $DIR
			FILES=`ls`
			for FILE in $FILES
			do
				$SWHOME/bin/swmake all $FILE
			done
				
			cd ..
				
			case $DIR in
			tux_bak)
				if [ ! -d "tux" ]
				then
					mkdir tux
				fi
				cp -rf ./tux_bak/* ./tux
				thirds tux TUXDIR2
			;;
			mqcom_bak)
				if [ ! -d "mq" ];then
					mkdir -p mq/com
				else
					mkdir   mq/com
				fi
				
				cp -rf ./mqcom_bak/* ./mq/com
				thirds mq MQCOMDIR
			;;
			mqc_bak)
				if [ ! -d "mq" ];then
					mkdir -p mq/mqc
				else
					mkdir  mq/mqc
				fi
				
				cp -rf ./mqc_bak/* ./mq/mqc
				thirds mq MQCDIR
#				if [ ! -d "mq/com" ];then
#					mkdir mq/com
#				fi
#				
#				mv mq/mqc/mq_pub.h mq/com/
			;;
			jmq_bak)
				if [ ! -d "mq" ];then
					mkdir -p mq/jmq
				else
					mkdir mq/jmq
				fi
				
				cp -rf ./jmq_bak/* ./mq/jmq
				thirds mq JMQDIR
#				if [ ! -d "mq/com" ];then
#					mkdir mq/com
#				fi
#				
#				mv mq/jmq/mq_pub.h mq/com/
				
			;;
			tea_bak)
				if [ ! -d "tea" ];then
					mkdir tea
				fi
				cp -rf ./tea_bak/* ./tea
				thirds tea TEADIR2
			;;
			tlq_bak)
				if [ ! -d "tlq" ];then
					mkdir tlq
				fi
				cp -rf ./tlq_bak/* ./tlq
				thirds tlq TLQDIR
			;;
			cics_bak)
				if [ ! -d "cics" ];then
					mkdir cics
				fi
				cp -rf ./cics_bak/* ./cics
				thirds cics CICSDIR
			;;
			esac
			
			check_trd_update
			cd $SWHOME/make
			cat sw.env | sed '/VERSION=/d' > .sw.env
			echo "VERSION=$VERSION" >> .sw.env
			mv .sw.env sw.env
			deal_upd_file $DIR
			if [ -z "$FILE" ];then
				echo "error:not find update lib!"
				exit -1
			else
				file=$FILE
			fi
			. $SWHOME/make/modules $file
			make 
			rm -f Makefile
			cd $SWHOME/src/plugin
		done
		cd $SWHOME
		rm -rf $SWHOME/src/plugin
	fi
	
	if [ -f "$SWHOME/src/udbc.tar" ]
	then
		cd $SWHOME/src
		mkdir udbc
		mv udbc.tar $SWHOME/src/udbc
		cd $SWHOME/src/udbc
		tar -xvf udbc.tar > /dev/null 2>&1
		rm -rf udbc.tar
		
		DIRS=`ls`
		for DIR in $DIRS
		do
			cd $DIR
			FILES=`ls`
			for FILE in $FILES
			do
				$SWHOME/bin/swmake all $FILE
			done
			cd ..
			
			case $DIR in
			ora_bak)
				if [ ! -d "ora" ]
				then
					mkdir ora
				fi
				cp -f ./ora_bak/* ./ora
				thirds ora ORACLE_HOME
			;;
			ifx_bak)
				if [ ! -d "ifx" ]
				then
					mkdir ifx
				fi
				cp -f ./ifx_bak/* ./ifx
				thirds ifx INFORMIXDIR
			;;
			db2_bak)
				if [ ! -d "odbc" ]
				then
					mkdir odbc 
				fi
				cp -f ./db2_bak/* ./odbc
				thirds db2 DB2_HOME
			;;
			esac
			check_trd_update
			cd $SWHOME/make
			cat sw.env | sed '/VERSION=/d' > .sw.env
			echo "VERSION=$VERSION" >> .sw.env
			mv .sw.env sw.env
			deal_upd_file $DIR
			if [ -z "$FILE" ];then
				echo "error:not find update lib!"
				exit -1
			else
				file=$FILE
			fi
			echo "udbc file=$file"
			echo "DIRLIST=$DIRLIST"
			. $SWHOME/make/modules $file
			make
			rm -f Makefile
			cd $SWHOME/src/udbc
		done	
		cd $SWHOME
		rm -rf $SWHOME/src/udbc		
	fi
	
	if [ -f "$SWHOME/src/file.tar" ]
	then
		cd $SWHOME/src
		mkdir -p plugin/file
		mv file.tar $SWHOME/src/plugin/file
		cd $SWHOME/src/plugin/file
		tar -xvf file.tar > /dev/null 2>&1
		rm -rf file.tar
		
		DIRS=`ls`
		for DIR in $DIRS
		do
			cd $DIR
			FILES=`ls`
			for FILE in $FILES
			do
				$SWHOME/bin/swmake all $FILE
			done
			cd ..
			
			case $DIR in
			tuxf_bak)
				if [ ! -d "tuxf" ]
				then
					mkdir tuxf
				fi
				cp -f ./tuxf_bak/* ./tuxf
				thirds tuxf TUXFILEDIR
			;;
			teaf_bak)
				if [ ! -d "teaf" ]
				then
					mkdir teaf
				fi
				cp -f ./teaf_bak/* ./teaf
				thirds teaf TEAFILEDIR
			;;
			esac
			check_trd_update
			cd $SWHOME/make
			cat sw.env | sed '/VERSION=/d' > .sw.env
			echo "VERSION=$VERSION" >> .sw.env
			mv .sw.env sw.env
			deal_upd_file $DIR
			if [ -z "$FILE" ];then
				echo "error:not find update lib!"
				exit -1
			else
				file=$FILE
			fi
			echo "file file=$file"
			echo "DIRLIST=$DIRLIST"
			. $SWHOME/make/modules $file
			make
			rm -f Makefile
			cd $SWHOME/src/plugin/file
		done	
		cd $SWHOME
		rm -rf $SWHOME/src/plugin/file		
	fi
	cat $SWHOME/etc/.updlist | sed '/^$/d' > $SWHOME/etc/.updlists
	mv $SWHOME/etc/.updlists $SWHOME/etc/.updlist
	echo "$upd_info|`date +%Y%m%d`" >> $SWHOME/etc/.updlist
	echo "$upd_info" | cut -d \| -f 1 >> $SWHOME/etc/.packlist
}

INSTALL 

echo "update plugin over!"

exit 0
