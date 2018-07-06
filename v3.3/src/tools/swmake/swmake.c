#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern int docryptfile(char *psInFile,char *psOutFile);

int main(int argc, char *argv[])
{
	int iRc;
        char sBuf[40];
	char sInc[256];
	char sFile[256];
	char sObj[100];
	
	memset(sBuf,0x00,sizeof(sBuf));
	memset(sInc, 0x00, sizeof(sInc));
	memset(sFile, 0x00, sizeof(sFile));
	memset(sObj, 0x00, sizeof(sObj));

	if(argc == 3)
	{
		if (strcmp(argv[1],"all") == 0)
		{
			sprintf(sFile, "%s",argv[2]);
			iRc = access(sFile, 0x00);
			if(iRc < 0)
			{
				printf("%s is not exist.\n",sFile);
				exit (1);
			}
			docryptfile(sFile, sFile);

			return 0;
		}

		if ((strcmp(argv[2],"0")== 0))
		{
			sprintf(sObj, "%s", argv[1]);
			iRc = access(sObj, 0x00);
			if(iRc < 0)
			{
				printf("%s is not exist.\n",sObj);
				exit (1);
			}
			docryptfile(sObj, sObj);

			return 0;	
		}
	}
	return -1;
}


