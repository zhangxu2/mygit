date=`date +%Y%m%d`

tmp_cleanup()
{
	cd $HOME
        rm -rf $SWHOME/src/$dir_info >/dev/null 2>&1
        exit 1;
}
trap "tmp_cleanup" 1 2 3 4 6 8 10 12 13 15

INSTALL()
{
	echo "now ready to update bin file, please wait ....."
	dd if="$0" of="$SWHOME/oriupd.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	if [ ! -d "$SWHOME" ];then
		echo "error:SWHOME path not exist,please checkout!"
		rm -rf $SWHOME/oriupd.tar.gz
		exit 0
	fi
	
	cd $SWHOME
	if [ -d "tmp" ];then
		mv tmp tmp.$date
	fi
	mkdir tmp
	
	mv oriupd.tar.gz $SWHOME/tmp
	cd $SWHOME/tmp
	gzip -d oriupd.tar.gz 
	tar -xf oriupd.tar 
	rm -f oriupd.tar 
	for dir in $dir_info
	do
		if [ ! -d "$SWHOME/src/$dir" ];then
			mv $SWHOME/tmp/$dir $SWHOME/src/
		fi
	done
	cd $SWHOME
	cnt=`ls tmp | wc -l`
	if [ $cnt -gt 0 ];then
		mv tmp/* src/
	fi
	rm -rf $SWHOME/tmp
	cd $SWHOME/make
	cp sw.env sw.env.$date
	version=`echo $upd_info | awk -F "|" '{print $2}'`
	echo "version=$version"
	echo "VERSION=$version" >> $SWHOME/make/sw.env
	dirflag=0
	for dir in $dir_info
	do
		if [ ! -d "$SWHOME/src/$dir" ];then
			echo "build dir:$dir not exist."
			rm -rf $SWHOME/make/sw.env
			mv $SWHOME/make/sw.env.$date $SWHOME/make/sw.env
			exit -1
		else
			cd $SWHOME/src/$dir
			dirflag=`expr $dirflag + 1`
			fileflag=0
			for file in $updfile
			do
				fileflag=`expr $fileflag + 1`
				if [ "$dirflag" = "$fileflag" ];then
					make clean;make $file
					break
				fi
			done
		fi
	done

	cd $SWHOME
	for dir in `ls src`
	do
		if [ "$dir" != "incl_pub" ];then
			rm -rf $SWHOME/src/$dir
		fi
	done
	
	rm -rf $SWHOME/make/sw.env
	mv $SWHOME/make/sw.env.$date $SWHOME/make/sw.env
	
	rm -rf $SWHOME/tmp 2>/dev/null
	cat $SWHOME/etc/.updlist | sed '/^$/d' > $SWHOME/etc/.updlists
	mv $SWHOME/etc/.updlists $SWHOME/etc/.updlist
	echo "$upd_info |`date +%Y%m%d`" >> $SWHOME/etc/.updlist
	echo "$upd_info" | cut -d \| -f 1 >> $SWHOME/etc/.packlist
}

INSTALL 

echo "update ori bin over!"

exit 0
