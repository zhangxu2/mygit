#include "msg_trans.h"

int main(int argc ,char *argv[])
{
	int ret;
	int fd;
	int len;
	int mqid;
	char tmp[64];
	sw_global_path_t path;

	memset(&path,0x00,sizeof(sw_global_path_t));

	cfg_set_path(&path);
	mqid = -1;
	fd = msg_trans_create(&path,IPC_PRIVATE, 1024,&mqid);
	if (fd < 0 || mqid <= 0)
	{
		printf("msg_trans_create [%d] \n",sw_errno);
		return 0;
	}
	printf("fd=[%d] mqid[%d] \n",fd,mqid);
	ret = msg_trans_send(fd,"hello world!",1,12);
	if (ret !=SW_OK)
	{
		printf("msg_trans_send [%d] \n",sw_errno);
		msg_trans_rm(&path,fd);
		return 0;
	}
	close(fd);

	fd = -1;
	fd = msg_trans_open(&path,mqid);
	if (fd < 0 )
	{
		printf("msg_trans_open [%d] \n",sw_errno);
		msg_trans_rm(&path,fd);
		return 0;
	}
	memset(tmp,0x00,sizeof(tmp));
	len = -1;
	long mtype=0;
	ret = msg_trans_rcv(fd,tmp,&mtype,&len);
	if (ret !=SW_OK || len < 0)
	{
		printf("msg_trans_rcv [%d]\n",sw_errno);
		msg_trans_rm(&path,fd);
		return 0;
	}
	
	printf("recv tmp=[%ld] [%d] [%s]\n",mtype,len,tmp);

	msg_trans_rm(&path,fd);

	return 0;
}
