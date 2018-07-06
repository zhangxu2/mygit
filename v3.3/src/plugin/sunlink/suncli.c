#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "SDKpub.h"
#include "tcpapi.h"

int main(int argc, char **argv)
{
	int	ret = 0;
	int	port = 0;
	FILE	*fp = NULL;
	char	ch = '\0';
	char	*ip = NULL;
	char	*ptr = NULL;
	char	recvbuf[1024*1024];
	char	*filename = NULL;
	TAPIHEAD	head;
	struct stat	filestat;
	
	memset(&head, 0x0, sizeof(TAPIHEAD));
	memset(&filestat, 0x0, sizeof(filestat));
	
	if (argc < 6)
	{
		printf("Usage:%s -i <IPADDR> -p <PORT> -d <DestNode> -n <NodeId> -t <TrType> -f <pkgname> \n", argv[0]);
		exit(1);
	}
	
	while (-1 != (ch = getopt(argc, argv, "i:p:d:n:t:f:")))
	{
		switch (ch)
		{
			case 'i':
				ip = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				strcpy(head.DestNode, optarg);
				break;
			case 'n':
				strcpy(head.NodeId, optarg);
				break;
			case 't':
				strcpy(head.TrType, optarg);
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}
	printf("ip=[%s] port=[%d] NodeId=[%s] DestNode=[%s] TrType=[%s]",
		ip, port, head.NodeId, head.DestNode, head.TrType);
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		printf("[%s][%d] Can not open file [%s]! errno=[%d]:[%s]\n",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	lstat(filename, &filestat);
	ptr = (char *)calloc(1, filestat.st_size + 1);
	if (ptr == NULL)
	{
		printf("[%s][%d] Calloc error! errno=[%d]:[%s]\n", 
			__FILE__, __LINE__, errno, strerror(errno));
		fclose(fp);
		return -1;
	}
	fread(ptr, 1, filestat.st_size, fp);
	fclose(fp);
	head.Sleng = strlen(ptr);

	head.PackInfo |= htonl( PI_VERSION_V2 );
	head.PackInfo |= htonl( PI_USESECU);

	
	printf("len=[%d] nodeid=[%s] destnode=[%s]\n", head.Sleng, head.NodeId, head.DestNode);
	printf("·¢ËÍÊý¾Ý:[%s]\n", ptr);
	memset(recvbuf, 0x0, sizeof(recvbuf));
	if (cli_sndrcv(ip, port, &head, ptr, NULL, recvbuf, NULL, 30) < 0)
	{
		printf("Communicate error! RetCode=[%s]\n", head.RetCode);
		return -1;
	}
	printf("Recv data:[%s][%d]\n", recvbuf, head.Sleng);
	free(ptr);

	return 0;
}

