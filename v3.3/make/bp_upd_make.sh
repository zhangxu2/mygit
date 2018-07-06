#!/bin/sh

sel_list()
{
cat << END
---------------------------------------------------------------
input params list:
---------------------------------------------------------------
os/unix           core                  pack
uniform           alert			admin 
job		  logs			comm
pol		  udbc/com   	        svc
lsn/map           lsn/comm 		lsn/tcp
lsn/svrmap	  udbc/ora		udbc/ifx
tools/cipc	  tools/swmake          tools/minfo  
tools/mipc        tools/txview          tools/swreg 
plugin/mq/com	  plugin/mq/mqc	 	plugin/mq/jmq
plugin/sunlink    plugin/tlq		plugin/des
plugin/udf        plugin/autosend       plugin/dealpkg
plugin/deny       plugin/file/tcpf      plugin/link
plugin/file/tuxf  plugin/jzcardpack     plugin/http
plugin/maptxcode  plugin/soap           plugin/start      
plugin/tea        plugin/tux            plugin/zip 	
plugin/allinpaysecure   udbc/odbc
---------------------------------------------------------------
end list params
---------------------------------------------------------------
END
}

deal_file()
{
dir=$1
file=$2

cd $dir
sed '/include $(SWHOME)\/make\/sw\.env/a\
VERSION='${version}'' $file >> tmp.a
mv tmp.a $file
make clean
make
}

echo "please input version:"
echo "eg: 3.1.1"
read version

echo "input dir in list:"
sel_list
read params

date=`date +%Y%m%d`

if [ ! -d "$SWHOME/src/$params" ];then
	echo "input dir not exist,please check."
	exit -1
else
	cd $SWHOME/src/$params
	if [ -f "Makefile" ];then
		cp Makefile Makefile"_"$date
		if [ -f "Makefile"_"$date" ];then
				deal_file $SWHOME/src/$params Makefile
				rm -f Makefile 
				mv Makefile"_"$date Makefile
		fi
	else
		echo "input dir not exist Makefile,please check."
		exit -1
	fi
fi

