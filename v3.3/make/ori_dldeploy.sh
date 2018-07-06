DATE=`date +%Y%m%d`
if [ -n "$SWHOME" ];then
	if [ -d "$SWHOME" ];then
		cd $HOME
		name=`echo $SWHOME | awk -F "/" '{print $NF}'`
		mv $SWHOME "$name"_$DATE.bak
		mkdir $name
	else
		echo "warning:profile already exist swhome,but homedir not exist, we will create default swhome!"
		mkdir $SWHOME
	fi
else
		cd $HOME
		if [ ! -d esw ];then
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

tmp_cleanup()
{
	cd $HOME
        rm -rf $SWHOME/src/* >/dev/null 2>&1
        exit 1;
}
trap "tmp_cleanup" 1 2 3 4 6 8 10 12 13 15
INSTALL()
{
	echo "now is copy file please waiting ....."
	dd if="$0" of="bp.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE >/dev/null 2>&1
	echo "copy file over .."
	echo "now is decbpression please waiting ....."
#	rm -f DFIS-BP.bin
	path=`pwd`
	if [ "$path" != "$SWHOME" ];then
		mv bp.tar.gz $SWHOME >/dev/null 2>&1
	fi
	cd $SWHOME
	gzip -d bp.tar.gz >/dev/null 2>&1
	tar xf bp.tar >/dev/null 2>&1
	rm -rf bp.tar >/dev/null 2>&1
	chmod -R 755 $SWHOME/bin
	chmod -R 755 $SWHOME/lib
	chmod -R 755 $SWHOME/plugin
	chmod -R 755 $SWHOME/make
	mkdir -p src
	mv src.tar src >/dev/null 2>&1
	cd $SWHOME/make
	rm -f bp* ori*
	if [ -f ".dirname" ];then 
		rm -f .dirname
	fi
	. $SWHOME/make/checkenv.sh
	cd $SWHOME
	echo "decbpression over..."
	
	echo "would you want build plugin[y/n]"
	read YY
	if [ "$YY" == "N" -o "$YY" = "n" ];then
		cd $SWHOME/src
		tar -xvf src.tar >/dev/null 2>&1 
		rm -f *.tar 
		echo "you have choice n, not need build third part!"
	else
		cd src
		tar -xvf src.tar >/dev/null 2>&1
		rm -f src.tar
		if [ -f "plugin.tar" ];then
			if [ ! -d "plugin" ];then
					mkdir -p plugin
			else
					cp -rf plugin  plugin.`date +%y%m%d`
					rm -rf ./plugin/*
			fi
			mv plugin.tar plugin/
		fi
		
		if [ -f "udbc.tar" ];then
				if [ ! -d "udbc" ];then
						mkdir -p udbc
				else
						cp -rf udbc  udbc.`date +%y%m%d`
						rm -rf ./udbc/*
				fi
			mv udbc.tar udbc/
		fi
		
		cd $SWHOME/make
		. $SWHOME/make/configure $bit $version
		
		if [ -f "$SWHOME/make/.dirname" ];then
			. $SWHOME/make/.dirname
		
			for dir in $DIRLIST
			do
					case $dir in
						ORACLE_HOME)
							cd $SWHOME/src/udbc
							tar -xvf udbc.tar ora >/dev/null 2>&1
							cd ora
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done	
						;;
						DB2_HOME)
							cd $SWHOME/src/udbc
							tar -xvf udbc.tar odbc >/dev/null 2>&1
							cd odbc
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done	
						;;
						TUXDIR)
							cd $SWHOME/src/plugin
							tar -xvf plugin.tar tux >/dev/null 2>&1
							cd tux
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done

							cd $SWHOME/src/plugin
							mkdir file
							tar -xvf plugin.tar file/tuxf >/dev/null 2>&1
							cd ./file/tuxf
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						MQDIR)
							cd $SWHOME/src/plugin
							tar -xvf plugin.tar mq >/dev/null 2>&1
						;;
						CICSDIR)
							cd $SWHOME/src/plugin
							tar -xvf plugin.tar cics >/dev/null 2>&1
							cd cics
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						TEADIR)
							cd $SWHOME/src/plugin
							tar -xvf plugin.tar tea >/dev/null 2>&1
							cd tea
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
							
							cd $SWHOME/src/plugin
							mkdir file
							tar -xvf plugin.tar file/teaf >/dev/null 2>&1
							cd ./file/teaf
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						TLQDIR)
						cd $SWHOME/src/plugin
							tar -xvf plugin.tar tlq >/dev/null 2>&1
							cd tlq
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						INFORMIXDIR)
							cd $SWHOME/src/udbc
							tar -xvf udbc.tar ifx >/dev/null 2>&1
							cd ifx
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						SUNDIR)
							cd $SWHOME/src/plugin
							tar -xvf plugin.tar sunlink >/dev/null 2>&1
							cd sunlink
							for FILE in `ls`
							do
								$SWHOME/bin/swmake all $FILE
							done
						;;
						esac
			done
		fi
		
		cd $SWHOME/make
		if [ -f ".dirname" ];then
			. $SWHOME/make/modules	
			make 
			rm -f $SWHOME/make/.dirname
		fi
	fi
	
	if [ -d "$SWHOME/src/plugin" ];then
		rm -rf $SWHOME/src/plugin
	fi
	if [ -d "$SWHOME/src/udbc" ];then 
		rm -rf $SWHOME/src/udbc
	fi
}

INSTALL
echo "Install success!"

exit 0

