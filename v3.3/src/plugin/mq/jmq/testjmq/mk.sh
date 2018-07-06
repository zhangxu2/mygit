
cc -brtl -o testjms testmqjms.c mq_pub.c -I. -I$SWHOME/src/incl_pub $SWHOME/lib/libswbase.so -D__SW_USE_JMS__ -I$JAVA_HOME/include -L$JVMLIBDIR -ljvm -L$NIOLIBDIR -lnio -L/usr/mqm/lib64 -lmqm
