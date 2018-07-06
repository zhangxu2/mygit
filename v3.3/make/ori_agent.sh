if [ $# -ne 1 ];then
	echo "err:input parameter errors,use the sample:sh agent.bin --b=32 or sh agent.bin --b=64"
	exit -1
fi

sys=`uname -s | tr [A-Z] [a-z]`
if [ "$sys" = "linux" ];then
	echo "please input system name, eg: susehost/redhost/sunhost"
	read sysname
	echo "please input system bit, eg: 32/64"
	read bit
	if [ "$sysname" != "redhost" -a "$sysname" != "susehost" -a "$sysname" != "sunhost" ];then
		echo "not suport system $sysname,please contact group version."
		exit 1
	fi

	if [ "$bit" != 32 -a "$bit" != "64" ];then
		echo "not suport system $bit, please contact group version."
		exit 1
	fi

	libs=`printf %s.%s.%s libstatgrab.a $sysname $bit`
	echo "libs=$libs"
fi

DATE=`date +%Y%m%d`
if [ -n "$SWHOME" ];then
	if [ -d "$SWHOME" ];then
		if [ -d "$SWHOME/bin" -a -d "$SWHOME/lib" -a -d "$SWHOME/plugin" -a "$SWHOME/include" -a "$SWHOME/bin/swadmin" ];then
			echo "bp exist, will install agent."
		else
			echo "bp not exit, please install bp."
			exit -1
		fi		
	else
		echo "profile already exist swhome,but homedir not exist, please install bp."
		exit 1
	fi
else
	
	echo "bp not exit, please install bp."
	exit -1
fi

tmp_cleanup()
{
	cd $HOME
        rm -rf $SWHOME/src/agent >/dev/null 2>&1
        exit 1;
}
trap "tmp_cleanup" 1 2 3 4 6 8 10 12 13 15

echo "now is install agent SWHOME=$SWHOME"

INSTALL()
{
	echo "now is copy file please waiting ....."
	dd if="$0" of="$SWHOME/agent.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	echo "copy file over .."
#	rm -f agent.bin >/dev/null 2>&1
	cd $SWHOME
	echo "now is decompression please waiting ....."
	if [ -d "tmp" ];then
		mv tmp tmp_bak
	fi
	
	mkdir tmp
	mv agent.tar.gz $SWHOME/tmp
	cd $SWHOME/tmp
	gzip -d agent.tar.gz >/dev/null 2>&1
	tar xf agent.tar lib sbin src/agent/include >/dev/null 2>&1
	mv  agent.tar $SWHOME/.agent.tar
	rm -rf agent.tar >/dev/null 2>&1
	cd $SWHOME

	if [  -d "$SWHOME/agent_lib" ];then
		mv $SWHOME/agent_lib $SWHOME/agent_lib_"$DATE"
	fi
	mkdir $SWHOME/agent_lib

	if [  -f "$SWHOME/lib/libstatgrab.a" ];then
		mv $SWHOME/lib/libstatgrab.a $SWHOME/lib/libstatgrab.a_"$DATE"	
	fi

	if [ "$sys" = "linux" ];then
		if [ ! -f "$SWHOME/tmp/lib/$libs" ];then
			echo "not exist $SWHOME/tmp/lib/$libs, exit"
			rm -rf $SWHOME/tmp
			rm -rf $SWHOME/agent_lib
			rm -f $SWHOME/.agent.tar
			exit 1
		else
			mv $SWHOME/tmp/lib/$libs $SWHOME/lib
			ln -s $SWHOME/lib/$libs $SWHOME/lib/libstatgrab.a
		fi
	fi

	if [  -d "$SWHOME/sbin" ];then
		mv $SWHOME/sbin $SWHOME/sbin_"$DATE"
	fi
	mv $SWHOME/tmp/sbin $SWHOME/sbin

	if [ -d "$SWHOME/src/agent" ];then
		mv $SWHOME/src/agent $SWHOME/src/agent_"$DATE"
	fi
	
	mkdir $SWHOME/src/agent
	mv $SWHOME/tmp/src/agent/include $SWHOME/src/agent
	rm -rf $SWHOME/tmp
	
	cd $SWHOME/make
	chmod 755 *
	rm -f bp* ori*
	. $SWHOME/make/checkenv.sh
	if [ $? -ne 0 ];then
		echo "checkenv error."
		cd $SWHOME
		rm -rf $SWHOME/agent_lib
		rm -f $SWHOME/.agent.tar
		rm -rf $SWHOME/src/agent
		exit 1
	fi
	
	. $SWHOME/make/agtcfg $1 $VERSION
	if [ $? -ne 0 ];then
		echo " agtcfg exit ,please check "
		cd $SWHOME
		rm -rf $SWHOME/agent_lib
		rm -f $SWHOME/.agent.tar
		rm -rf $SWHOME/src/agent
		exit 1
	fi
	
	cd $SWHOME
	dirlist="
		comm
		admin
		pol
		main
		tx
		"
	
	for item in $dirlist
	do
		curpath=$SWHOME/src/agent/$item
		cd $SWHOME
		tar -xf .agent.tar src/agent/$item
		cd $curpath

		make clean
		if [ "$item" = "tx" -a "$sys" = "sunos" ];then
			make -f Makefile.solaris
		else
			make
		fi
		if [ $? -ne 0 ];then
			cd $SWHOME
			rm -rf $SWHOME/agent_lib
			rm -f $SWHOME/.agent.tar
			rm -rf $SWHOME/src/agent
			exit 1
		fi
		cd $SWHOME
		rm -rf $curpath
			
	done
	rm -rf $SWHOME/src/agent
	rm -f $SWHOME/.agent.tar
}

INSTALL $1

echo "deploy agent  over!"

exit 0
 
