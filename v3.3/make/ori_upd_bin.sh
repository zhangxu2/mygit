
INSTALL()
{
	echo "now ready to update bin file, please wait ....."
	dd if="$0" of="$SWHOME/bin.tar.gz" bs=1 skip=$SHELL_SIZE count=$FILE_SIZE > /dev/null 2>&1
	if [ ! -d "$SWHOME" ];then
		echo "error:SWHOME path not exist£¬ please checkout!"
		exit 0
	fi
	
	cd $SWHOME
	if [ -d "tmp" ];then
		mv tmp tmp.`date +%Y%m%d`
	fi
	mkdir tmp
	
	mv bin.tar.gz $SWHOME/tmp
	cd $SWHOME/tmp
	gzip -d bin.tar.gz 
	tar -xvf bin.tar 
	rm -f bin.tar 
	for list in `ls`
	do
		cd $SWHOME/bin
		s=`echo $list | cut -d '.' -f 1`
		if [ ! -f "$list" ];then
			if [ -f "$s" ];then
				mv $s $s.`date +%Y%m%d`
				mv $SWHOME/tmp/$list ./
				ln -s $list $s
			else
				mv $SWHOME/tmp/$list ./
				ln -s $list $s
			fi	
		fi
	done
	
	rm -rf $SWHOME/tmp 2>/dev/null
	cat $SWHOME/etc/.updlist | sed '/^$/d' > $SWHOME/etc/.updlists
	mv $SWHOME/etc/.updlists $SWHOME/etc/.updlist
	echo "$upd_info |`date +%Y%m%d`" >> $SWHOME/etc/.updlist
	echo "$upd_info" | cut -d \| -f 1 >> $SWHOME/etc/.packlist
}

INSTALL 

echo "update bin over!"

exit 0
