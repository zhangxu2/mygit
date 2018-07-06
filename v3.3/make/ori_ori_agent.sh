date=`date +%Y%m%d`

tmp_cleanup()
{
	cd $HOME
        rm -rf $SWHOME/src/agent >/dev/null 2>&1
        exit 1;
}
trap "tmp_cleanup" 1 2 3 4 6 8 10 12 13 15

INSTALL()
{
	echo "now ready to update bin file, please wait ....."
	dd if="$0" of="$SWHOME/oriagent.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	if [ ! -d "$SWHOME" ];then
		echo "error:SWHOME path not exist,please checkout!"
		rm -rf $SWHOME/oriagent.tar.gz
		exit 0
	fi
	
	cd $SWHOME
	if [ -d "tmp" ];then
		mv tmp tmp.$date
	fi
	mkdir tmp
	
	mv oriagent.tar.gz $SWHOME/tmp
	cd $SWHOME/tmp
	gzip -d oriagent.tar.gz 
	tar -xf oriagent.tar 
	rm -f oriagent.tar 
	if [ -d "$SWHOME/src/agent" ];then
		mv $SWHOME/src/agent $SWHOME/src/agent_"$date"
	fi

	mkdir $SWHOME/src/agent
	mv $SWHOME/tmp/* $SWHOME/src/agent

	cd $SWHOME
	rm -rf $SWHOME/tmp 2>/dev/null

	dirlist="
	agent/comm
	agent/main
	agent/admin
	agent/pol
	agent/tx
	"

	cp $SWHOME/make/agtenv $SWHOME/make/agtenv.$date
	echo "VERSION=$version" >> $SWHOME/make/agtenv
	cd $SWHOME/src
	for file in $updfile
	do
		for dir in $dirlist
		do
			curpath=$SWHOME/src/$dir
			grep $file $curpath/Makefile 2>/dev/null
			if [ $? -eq 0 ];then
				if [ "$dir" = "agent/admin" -o "$dir" = "agent/pol" ];then
                                        cp $SWHOME/bin/$file $SWHOME/bin/"$file"_"$date"
				elif [ "$dir" = "agent/tx" ];then
					cp $SWHOME/agent_lib/$file $SWHOME/agent_lib/"$file"_"$date"
				elif [ "$dir" = "agent/main" ];then
					s=`echo $file | awk '{if ($0~/\.so/) print 1}'`
					if [ "$s" = "1" ];then
                                                cp $SWHOME/lib/$file $SWHOME/lib/"$file"_"$date"
                                        else
                                                cp $SWHOME/bin/$file $SWHOME/bin/"$file"_"$date"
                                        fi
				else
                                        echo " path $dir error."
                                        cd $SWHOME
                                        rm -rf $SWHOME/make/agtenv
                        		mv $SWHOME/make/agtenv.$date $SWHOME/make/agtenv
                                        rm -f $SWHOME/src/agent
                                        exit 1
				fi
				cd $curpath
				sys=`uname -s | tr [A-Z] [a-z]`
        			if [ "$sys" = "sunos" -a "$curpath" = "tx" ];then
					make -f Makefile.solaris clean
					make -f Makefile.solaris $file
				else
					make clean;make $file
				fi

				break
			fi
		done
	done

	cd $SWHOME
	rm -rf $SWHOME/make/agtenv
	mv $SWHOME/make/agtenv.$date $SWHOME/make/agtenv
	rm -rf $SWHOME/src/agent
}

INSTALL 

echo "update ori bin over!"

exit 0
