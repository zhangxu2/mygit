#!/bin/sh

function date2days {
	echo "$1 $2 $3" | awk '{
		z=int((14-$2)/12); y=$1+4800-z; m=$2+12*z-3;
		j=int((153*m+2)/5)+$3+y*365+int(y/4)-int(y/100)+int(y/400)-2472633;
		print j
		}'
}

function days2date {
	echo "$1" | awk '{
		a=$1+2472632; b=int((4*a+3)/146097); c=int((-b*146097)/4)+a; 
		d=int((4*c+3)/1461); e=int((-1461*d)/4)+c; m=int((5*e+2)/153);
		dd=-int((153*m+2)/5)+e+1; mm=int(-m/10)*12+m+3; yy=b*100+d-4800+int(m/10);
		printf ("%4d%02d%02d\n",yy,mm,dd)
		}'
}

function get_time {
year=`date +%Y`; month=`date +%m`; day=`date +%d`
days=`date2days $year $month $day`

N=$1
let days-=$N
days2date $days
}

function date2seconds {
    echo "$*" | awk '{
        z=int((14-$2)/12); y=$1+4800-z; m=$2+12*z-3;
        j=int((153*m+2)/5)+$3+y*365+int(y/4)-int(y/100)+int(y/400)-2472633;
        j=j*86400+$4*3600+$5*60+$6
        print j
    }'
}

function changetime {
		echo "$*" | awk '{
			s=int($1+$2*3600+$3*60+$4)
			printf s
		}'
}

function cuttime {
		eval $2=`echo $1|cut -b 1-2`
		eval $3=`echo $1|cut -b 3-4`
		eval $4=`echo $1|cut -b 5-6`
}

function seconds2date {
    echo "$1" | awk '{
        i=$1; ss=i%60; i=int(i/60); nn=i%60; i=int(i/60); hh=i%24; dd=int(i/24); i=int(i/24);
        a=i+2472632; b=int((4*a+3)/146097); c=int((-b*146097)/4)+a; 
        d=int((4*c+3)/1461); e=int((-1461*d)/4)+c; m=int((5*e+2)/153);
        dd=-int((153*m+2)/5)+e+1; mm=int(-m/10)*12+m+3; yy=b*100+d-4800+int(m/10);
        printf ("%4d-%02d-%02d %02d:%02d:%02d\n",yy,mm,dd,hh,nn,ss)
    }'
}


argc=$#
if [ $argc -lt 3 ]
then
	echo "err: use: get_trc.sh page_idx page_cnt date[]\n"
fi

page_idx=$1
page_cnt=$2
conditon=$3
start_t=$4
end_t=$5


date_time=`get_time 0`

aaa=`date +%s`

echo $aaa
 
myday=`seconds2date $aaa`
echo "bbbbb" $myday


if [ "$start_t" != "null" -a "$end_t" != "null" ]
then
	cuttime $start_t HS MS SS
	cuttime $end_t HE ME SE
	
	#echo "$HS $MS $SS"
	
	
	Y=`date +%Y`
	M=`date +%m`
	D=`date +%d`

	second=`date2seconds $Y $M $D`
	
	start_s=`changetime $second $HS $MS $SS`
	end_s=`changetime $second $HE $ME $SE`
	
	
	myday=`seconds2date $start_s`
	echo "aaaaaaaa" $myday
	
	start_s=`echo "scale=0;$start_s*1000000"|bc`
	end_s=`echo "scale=0;$end_s*1000000"|bc`
	
	echo $start_s
	
fi

if [ -z $SWWORK ]
then
	echo "err: No env SWWORK!"
	exit -1
fi

log_path=$SWWORK/tmp/monitor/$date_time/monitor.log
if [ ! -f $log_path ]
then
	echo "err: no file\[$log_path\]!"
	exit -1
fi

i=0
while myline=$(line)
do
	if [ "$conditon" = "null" -o  -n "$(echo $myline|grep "$conditon" )" ]
	then
		if [ "$(echo $myline|awk '{if($0~/TOTAL/) print 1}')" = "1" ]
		then
			myline=$(echo $myline | sed -e 's/TOTAL://g' -e 's/ //g')	
			if [ "$start_t" != "null" -a "$end_t" != "null" ]
			then
				n=`echo "$myline" | awk '{print split($0, item, "|")}'`
				i=0
				j=0
				while [ $i -le $n ]
				do
					i=`expr $i + 1`
					buf=`echo $myline | cut -d \| -f$i`
					item[$j]=$buf
					j=`expr $j + 1`
				done
				start=${item[5]}
				end=${item[6]}
				
				#echo $start_s $end_s $start 
				
				#start=`echo "scale=0;$start/1000000"|bc`
				#myday=`seconds2date $start`
				#echo "aaaaaaaa" $myday
				
				
				
				if [ $start -ge $start_s -a $start -le $end_s ]
				then
					#echo $myline
					out[$i]=$myline
				fi
				
			else
				out[$i]=$myline
			fi
			i=`expr $i + 1`
		fi
	fi
done < $log_path

cnt=${#out[@]}
if [ $cnt -eq 0 ]
then
	echo "page_sum:0"
	exit 0
fi

page_sum=0
if [ $(($cnt%$page_cnt)) -eq 0 ]
then
	page_sum=$(($cnt/$page_cnt))
else
	page_sum=`expr $(($cnt/$page_cnt)) + 1`
fi

echo "page_sum:$page_sum"

if [ $page_idx -gt $page_sum ]
then
	echo "page_idx\[$page_idx\] is out of range!"
	exit -1
fi

low=$(($(($(($page_idx - 1))*$page_cnt))+1))
high=$(($page_idx*$page_cnt))


if [ $high -gt $cnt ]
then
	high=$cnt
fi

k=0
i=1
while [ $i -le $cnt ]
do
	if [ $i -ge $low -a $i -le $high ]
	then
		j=`expr $i - 1`
		date[$k]=${out[$j]}
		k=`expr $k + 1`
	fi
	i=`expr $i + 1`
done

count=${#date[@]}
echo "data_cnt:$count";
for var in ${date[@]}
do
	n=`echo "$var" | awk '{print split($0, item, "|")}'`
	i=0
	j=0
	while [ $i -le $n ]
	do
		i=`expr $i + 1`
		buf=`echo $var | cut -d \| -f$i`
		item[$j]=$buf
		j=`expr $j + 1`
	done

	tx_code=${item[0]}
	trcno=${item[1]}
	svr=${item[2]}
	svc=${item[3]}
	date=${item[4]}
	start=${item[5]}
	end=${item[6]}
	
	cost=0
	if [ -z "$start" -a -z "$end" ]
	then
		cost=$(($end-$start))
	fi
	
	respcd=${item[7]}
	prdt=${item[8]}
	
	txamount=${item[9]}
	deaccount=${item[10]}
	craccount=${item[11]}
	deaccname=${item[12]}
	craccname=${item[13]}
	poundage=${item[14]}
	chaccount=${item[15]}
	nowturnsign=${item[16]}
	borrowmark=${item[17]}
	achannel=${item[18]}
	txtime=${item[19]}
	errcode=${item[20]}
	resmsg=${item[21]}
	bustrcno=${item[22]}
	
	if [ -z "$bustrcno" ]
	then
		bustrcno="";
	fi
	
	echo "tx_code[$tx_code]trcno[$trcno]svr[$svr]svc[$svc]date[$date]\
start[$start]end[$end]cost[$cost]respcd[$respcd]prdt[$prdt]\
txamount[$txamount]deaccount[$deaccount]craccount[$craccount]deaccname[$deaccname]\
craccname[$craccname]poundage[$poundage]chaccount[$chaccount]nowturnsign[$nowturnsign]\
borrowmark[$borrowmark]achannel[$achannel]txtime[$txtime]errcode[$errcode]resmsg[$resmsg]bustrcno[$bustrcno]"
done

exit 0
