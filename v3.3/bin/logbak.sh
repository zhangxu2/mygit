#!/bin/sh
if [ "$5" == "-u" ]; then
	LIST=`find $1 -name "*$3*" | sed 's/.*\/log\///g' `

	mkdir -p $2/$3

	cd $1
	tar -cf $2/$3/$3.tar $LIST syslog
	gzip -f $2/$3/$3.tar
	
	rm -f $LIST

	day=`echo "$4" | sed 's/.*d//g'`
	
	cd $2

	for file in `ls`
	do 
		str="$2/$file"
		str=${str##*\/}
		str=${str%%.*}

		result=`expr $3 - $str`
	
		if [ $result -gt $day ];then
			rm -rf $str*
		fi
	done
fi
