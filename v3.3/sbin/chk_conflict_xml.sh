#!/bin/sh
touch tmp
function get_file_size
{
	filename=$1
	ret=-1
	if [ ! -s $filename ]
	then
		echo "err:[$0][$LINENO]Param error"
		exit $ret
	fi


	file_size=`ls -l $filename`
	echo $file_size | awk '{print $5;}' > tmp
}

function cmp_dir
{
	> 1.px	
	bpdir=$1
	tardir=$2
	i=0
	if [ -z "$bpdir" ] || [ -z "$tardir" ]
	then
		echo "err:"__FILE__,__LINE__" Param error"
		renturn $ret
	fi
	if [ ! -d "$bpdir" ]
	then
		echo "[$0][$LINENO]No dir !!!"
	fi
	if [ ! -d "$tardir" ]
	then
		echo "[$0][$LINENO]No tar !!!"
	fi

	for bp_ab_name in ` ls $bpdir `
	do
		for tar_ab_name in ` ls $tardir `
		do
			if [ $bp_ab_name = $tar_ab_name ]
			then
				get_file_size $bpdir/$bp_ab_name
				bp_size=`cat tmp`
				get_file_size $tardir/$tar_ab_name
				tar_size=`cat tmp`
				rm -f tmp
					
				if [ $bp_size -ne $tar_size ]
				then
					echo "$bpdir/$bp_ab_name" >> 1.px
				fi			
			fi
		done
	done
}

argc=$#

if [ $argc -ne 2 ] && [ $argc -ne 3 ]
then
	echo "err:[$0][$LINENO]chk_conflict_xml.pl install_dir out_xml_path [prdt_name]"
	exit -1
fi

install=$1
if [ ! -d $install ]
then
	echo "err:[$0][$LINENO]No dir $install!"
	exit -1
fi

out_xml=$2
result=0
prdt=$3

swwork=$SWWORK
if [ ! -d $swwork ]
then
	echo "err:[$0][$LINENO] No env SWWORK!"
	exit -1
fi

dirs="
lib
bin
plugin"

touch contmp

for dir in ${dirs[@]}
do
	bp_dir=$swwork/$dir
	tar_dir=$install/$dir
	if [ -d $bp_dir ] && [ -d $tar_dir ]
	then
		cmp_dir $bp_dir $tar_dir


		size=`ls -l 1.px`
		echo $size | awk '{print $5}' > pxsize

		result=`cat pxsize` 
		if [ $result -ne 0 ]
		then
			while read line 
			do
				echo "$line" >> contmp
			done < 1.px
		fi
	fi
	rm -f 1.px
done

if [ ! -z $prdt ]
then
	for dir in ${dirs[@]}
	do
		bp_prdt_dir=$swwork/products/$prdt/$dir
		tar_prdt_dir=$install/products/$prdt/$dir
		if [ -d $bp_prdt_dir ] && [ -d $tar_prdt_dir ]
		then
			cmp_dir $bp_prdt_dir $tar_prdt_dir
                	
     			size=`ls -l 1.px`
			echo $size | awk '{print $5}' > pxsize

			result=`cat pxsize` 
			if [ $result -ne 0 ]
			then
				while read line 
				do
					echo $line | grep ".sql" > /dev/null
					if [ $? -ne 0 ]
					then
						echo "$line" >> contmp
					fi
				done < 1.px
			fi
         
		fi
	done
fi

echo "<Conflicts>" > $out_xml
	while read conflict
	do
		len1=${#swwork}
		len2=`expr $len1 + 2`
		len3=${#conflict}
		cname=`expr substr "$conflict" $len2 $len3`
		echo "	<Conflict>" >> "$out_xml"
		echo "		<pt_path>$cname</pt_path>" >> $out_xml
		echo "		<pkg_path>$cname</pkg_path>" >> $out_xml
		echo "	</Conflict>" >> $out_xml
	done < contmp

rm -f contmp
echo "</Conflicts>" >> $out_xml
echo "ok:[$0][$LINENO]ok: check conflict finished!"
exit 0
