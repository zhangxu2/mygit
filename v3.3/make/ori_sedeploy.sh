if [ $# -ne 1 ];then
	echo "err:input parameter errors,use the sample:sh bp.bin --b=32 or sh bp.bin --b=64"
	exit -1
fi

SW_ENV=sw.env
DATE=`date +%Y%m%d`

if [ -n "$SWHOME" ];then
	if [ -d "$SWHOME" ];then
		cd $SWHOME
		cd ..
		name=`echo $SWHOME|awk -F"/" '{print $NF}'`
		cp -rf $SWHOME "$name"_$DATE.bak
	else
		echo "profile already exist swhome,but homedir not exist, we will create swhome!"
                mkdir $SWHOME
	fi
else
	echo "SWHOME not exist, create default SWHOME=$HOME/esw!"
	cd $HOME
	if [ ! -d esw ];then
		mkdir esw
	else
		mv esw esw_$DATE.bak
		mkdir esw
	fi
		
	if [ -f ~/.profile ];then
		echo 'export SWHOME=$HOME/esw' >> ~/.profile
		. ~/.profile
	fi
		
	if [ -f ~/.bash_profile ];then
		echo 'export SWHOME=$HOME/esw' >> ~/.bash_profile
		. ~/.bash_profile
	fi
fi

CUR_DIR=`pwd`
tmp_cleanup()
{
	cd $HOME
        rm -rf $SWHOME/* >/dev/null 2>&1
        exit 1;
}
trap "tmp_cleanup" 1 2 3 4 6 8 10 12 13 15

echo "now is install DFIS_BP SWHOME=$SWHOME"

INSTALL()
{
	echo "now is copy file please waiting ....."
	dd if="$0" of="$SWHOME/DFIS_BP.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	echo "copy file over .."
#	rm -f bp.bin >/dev/null 2>&1
	cd $SWHOME
	echo "now is decompression please waiting ....."
	gzip -d DFIS_BP.tar.gz >/dev/null 2>&1
	tar xf DFIS_BP.tar include make plugin etc bin src/incl_pub >/dev/null 2>&1
	mv DFIS_BP.tar $SWHOME/.DFIS.tar
	rm -rf DFIS_BP.tar >/dev/null 2>&1
	if [ ! -d $SWHOME/lib ];then
		mkdir $SWHOME/lib
	fi

	cd $SWHOME/make
	chmod 755 *
	rm -f bp* ori*
	. $SWHOME/make/checkenv.sh
	if [ $? -ne 0 ];then
		echo "checkenv error."
		rm -rf $SWHOME/src
		rm -rf $SWHOME/.DFIS.tar 
	fi
	
	. $SWHOME/make/configure $1 $VERSION
	if [ $? -ne 0 ];then
		echo " configure exit ,please check "
		rm -rf $SWHOME/src
		rm -rf $SWHOME/.DFIS.tar
		exit 1
	fi
	
	. $SWHOME/make/swbuild d no $SWHOME/.DFIS.tar
	if [ $? -ne 0 ];then
		echo " makeall exit ,please check "
		rm -rf $SWHOME/src
		rm -rf $SWHOME/.DFIS.tar
		exit 1
	fi
	
	cd $SWHOME
	tar xf .DFIS.tar src/plugin src/udbc
	rm -f .DFIS.tar
	cd $SWHOME/make

	if [ -f $SWHOME/make/.dirname ];then
		echo "Do you want to build to a third party plugin library,y/n?"
		read answer
		if [ "$answer" = "y" -o "$answer" = "Y" ];then
			if [ ! -f $SWHOME/make/.dirname ];then
				echo "not need build third party document!"
				cd $SWHOME
				rm -rf $SWHOME/src
				exit 0
			fi
				
			. $SWHOME/make/.dirname
			.  $SWHOME/make/modules
			if [ $? -ne 0 ];then
				echo " modules error."
				cd $SWHOME
				rm -rf $SWHOME/src
				rm -f $SWHOME/make/Makefile
				exit 1
			fi
			make
			rm -f $SWHOME/make/.dirname
			rm -f $SWHOME/make/Makefile
		fi
	fi

	cd $SWHOME	
	cp -rf $SWHOME/src/incl_pub $SWHOME
	rm -rf $SWHOME/src/*
	mv $SWHOME/incl_pub $SWHOME/src/
}

INSTALL $1

echo "deploy DFIS_BP over!"

exit 0

