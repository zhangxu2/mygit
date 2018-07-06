#Run all the test exe file.

./uni_svr_test
if [ $? -ne 0 ]
then
	echo "uni_svr_test fail!"
else
	echo "uni_svr_test success!"
fi

./uni_swconfig_test
if [ $? -ne 0 ]
then
	echo "uni_swconfig_test fail!"
else
	echo "uni_swconfig_test success!"
fi

./uni_dbconfig_test
if [ $? -ne 0 ]
then
	echo "uni_dbconfig_test fail!"
else
	echo "uni_dbconfig_test success!"
fi

./uni_cluster_cfg_test
if [ $? -ne 0 ]
then
	echo "uni_cluster_cfg_test fail!"
else
	echo "uni_cluster_cfg_test success!"
fi

./uni_trace_test
if [ $? -ne 0 ]
then
	echo "uni_trace_test fail!"
else
	echo "uni_trace_test success!"
fi