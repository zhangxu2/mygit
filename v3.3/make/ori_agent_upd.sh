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
	if [ ! -d "$SWHOME/src/agent" ];then
		mkdir $SWHOME/src/agent
		mv $SWHOME/tmp/* $SWHOME/src/agent
	fi

	cd $SWHOME
	rm -rf $SWHOME/tmp 2>/dev/null
	cd $SWHOME/src/agent
	for file in $updfile
	do
		cd tx
		make clean;make $file
	done

	cd $SWHOME
	rm -rf $SWHOME/src/agent
	cat $SWHOME/etc/.updlist | sed '/^$/d' > $SWHOME/etc/.updlists
	mv $SWHOME/etc/.updlists $SWHOME/etc/.updlist
	echo "$upd_info |`date +%Y%m%d`" >> $SWHOME/etc/.updlist
	echo "$upd_info" | cut -d \| -f 1 >> $SWHOME/etc/.packlist
}

INSTALL 

echo "update ori bin over!"

exit 0
