INSTALL()
{
	echo "now ready to update lib file, please wait ....."
	dd if="$0" of="$SWHOME/lib.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	if [ ! -d "$SWHOME" ];then
		echo "error:SWHOME path not exist£¬ please checkout!"
		exit 0
	fi
	
	cd $SWHOME
	if [ -d "tmp" ];then
		mv tmp tmp.`date +%Y%m%d`
	fi
	mkdir tmp
	
	mv lib.tar.gz ./tmp 
	cd tmp
	gzip -d lib.tar.gz 
	tar -xvf lib.tar 
	rm -f lib.tar
	
	dirlist="lib plugin"
	for list in `ls`
	do
		for dir in $dirlist
		do
			flag=0
			cd $SWHOME/$dir
			for base in `ls`
			do
				s=`echo $list | cut -d '.' -f 1,2`
				if [ ! -f "$list" ];then
					if [ -f "$s" ];then
						mv $s $s.`date +%Y%m%d`
						mv $SWHOME/tmp/$list ./
						ln -s $list $s
						flag=1
					fi
				fi
				
				if [ "$flag" -eq 1 ];then
					break
				fi
			done
			
			if [ "$flag" -eq 1 ];then
				break
			fi
		done
		cd $SWHOME/tmp
	done

	cd $SWHOME	
	rm -rf $SWHOME/tmp 2>/dev/null
	cat $SWHOME/etc/.updlist | sed '/^$/d' > $SWHOME/etc/.updlists
	mv $SWHOME/etc/.updlists $SWHOME/etc/.updlist
	echo "$upd_info|`date +%Y%m%d`" >> $SWHOME/etc/.updlist
	echo "$upd_info" | cut -d \| -f 1 >> $SWHOME/etc/.packlist
}

INSTALL 

echo "update lib over!"

exit 0
