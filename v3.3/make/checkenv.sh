#!/bin/sh -f

############################
# 环境变量检查 #
############################
printf "环境变量检查开始...\n"

flag=0
PROF=

if [ -f ~/.profile ];then
	PROF=~/.profile
	flag=0
fi

if [ -f ~/.bash_profile ]
then
	PROF=~/.bash_profile
	flag=1
fi

ENVLIST="
SWHOME
ORACLE_HOME
ORACLE_LIBDIR
"

for Env in $ENVLIST
do
	#ENV=`env "$Env"`
	eval Envs=\$$Env
		if [  -z $Envs ];then
			if [ $Env = 'SWHOME' ];then
				echo 'export SWHOME=$HOME/esw' >> $PROF
			elif [ $Env = 'ORACLE_HOME' ];then
				echo 'export ORACLE_HOME=$ORACLE_HOME' >> $PROF
			elif [ $Env = 'ORACLE_LIBDIR' ];then
				if [ -d $ORACLE_HOME/lib64 ];then
					echo "export ORACLE_LIBDIR=lib64" >> $PROF
				elif [ -d $ORACLE_HOME/lib ];then
					echo "export ORACLE_LIBDIR=lib" >> $PROF
				elif [ -d $ORACLE_HOME/lib32 ];then
					echo "export ORACLE_LIBDIR=lib32" >> $PROF
				else
					echo "[ORACLE_HOME]设置有误,请检查!"
					exit 1
				fi
			else
				echo "环境变量["$Env"]未配置,请进行配置!"
				exit 1
			fi
		fi
done

LIBLIST='
$SWHOME/lib
$SWHOME/plugin
$ORACLE_HOME/$ORACLE_LIBDIR
'
sys=`uname -s | tr [A-Z] [a-z]`
if [ "$sys" = "linux" -o "$sys" = "sunos" ];then
	LIBENV='$LD_LIBRARY_PATH'
	path=LD_LIBRARY_PATH
else
	LIBENV='$LIBPATH'
	path=LIBPATH
fi

for LIB in $LIBLIST
do
	grep $LIB $PROF|grep $path > /dev/null
	if [ $? -ne 0 ];then
		LIBENV=$LIBENV:$LIB
	fi
done

if [ "$sys" = "linux" -o "$sys" = "sunos" ];then
	if [ $LIBENV != '$LD_LIBRARY_PATH' ];then
		echo "export LD_LIBRARY_PATH=$LIBENV" >> $PROF
	fi
else
	if [ $LIBENV != '$LIBPATH' ];then
		echo "export LIBPATH=$LIBENV" >> $PROF
	fi
fi


BINLIST='
$SWHOME/bin
$ORACLE_HOME/bin
'

BINENV='$PATH'
for BIN in $BINLIST
do
	grep $BIN $PROF| grep PATH| grep -v LIBPATH | grep -v LD_LIBRARY_PATH > /dev/null
	if [ $? -ne 0 ];then
		BINENV=$BINENV:$BIN
	fi
done

if [ $BINENV != '$PATH' ];then
	echo "export PATH=$BINENV" >> $PROF
fi

. $PROF

echo "环境变量检查完成!"
