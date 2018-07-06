#!/bin/sh

if [ $# -ne 1 ];then
	echo "usage: upd_pack.sh 3.0.0"
	exit -1
fi

listhead()
{
cat <<END

Base Version:[$version]

 No                     Updlist
--------------------------------------
END
}

listbody()
{
cat << END
[$id]                  [$packinfo]
END
}

listtail()
{
cat << END
--------------------------------------
END
}

uselist()
{
cat << END

Usage:
--------------------------------------
detail info input sample: pack001
direct quit input sample: quit 
--------------------------------------
END
}

version=$1
listhead
i=0 
id=0

if [ ! -s "$SWHOME/etc/.packlist" ];then
	listtail
	echo "pack list file is null, please check!"
	exit -1
fi

cat $SWHOME/etc/.packlist | while read line 
do
	i=`expr $i + 1`
	id=`printf "%03d" $i`
	packinfo=$line
	listbody
done

listtail

uselist

read answer

if [ "$answer" = "quit" -o "$answer" = "exit" ];then
	exit 0
elif cat $SWHOME/etc/.packlist | grep "$answer" 
then
	sh $SWHOME/make/upd_list.sh $answer
else
	echo "Please input the correct number!"
fi

