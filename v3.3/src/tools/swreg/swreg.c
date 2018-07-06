#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "pub_log.h"

#define LICFILE	"cfg/.license.dat"

static int reg(char *lic)
{
	int	len = 0;
	int	flag = 0;
	FILE	*fp = NULL;
	char	line[512];
	char	filename[128];
	
	if (getenv("SWWORK") == NULL)
	{
		printf("Î´ÉèÖÃ»·¾³±äÁ¿SWWORK!\n");
		return -1;
	}
	memset(filename, 0x0, sizeof(filename));
	sprintf(filename, "%s/%s", getenv("SWWORK"), LICFILE);
	fp = fopen(filename, "a+");
	if (fp == NULL)
	{
		printf("Can not open file [%s]! errno=[%d]:[%s]", filename, errno, strerror(errno));
		return -1;
	}
	
	flag = 0;
	while (1)
	{
		memset(line, 0x0, sizeof(line));	
		if (fgets(line, sizeof(line), fp) == NULL)
		{
			break;
		}
		len = strlen(line);
		if (line[len - 1] == '\n')
		{
			line[len - 1] = '\0';
		}
		
		if (strcmp(line, lic) == 0)
		{
			flag = 1;
			break;
		}
	}
	
	if (flag == 1)
	{
		printf("×¢²áÂë[%s]ÒÑ×¢²á!\n", lic);
	}
	else
	{
		fprintf(fp, "%s\n", lic);
	}
	fclose(fp);
	
	return 0;
}

int main(int argc, char **argv)
{
	int	len = 0;
	int	ret = 0;
	char	line[128];
	
	if (argc > 1)
	{
		if (strcmp(argv[1], "-r") == 0)
		{
			ret = reg(argv[2]);
			if (ret < 0)
			{
				printf("×¢²áÊ§°Ü!\n");
				return -1;
			}
			printf("×¢²á³É¹¦!\n");
			return 0;
		}
	}
	
	printf("ÇëÊäÈë×¢²áÂë:");
	memset(line, 0x0, sizeof(line));
	fgets(line, sizeof(line), stdin);
	len = strlen(line);
	if (line[len - 1] == '\n')
	{
		line[len - 1] = '\0';
	}
	
	ret = reg(line);
	if (ret < 0)
	{
		printf("×¢²áÊ§°Ü!\n");
		return -1;
	}
	
	printf("×¢²á³É¹¦!\n");
	return 0;
}
