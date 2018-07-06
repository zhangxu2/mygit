#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int docryptfile(char *psInFile,char *psOutFile)
{
        FILE *fp1;
        FILE *fp2;
        int i;
        char *psBuf;
        int iLen;
        char sPwd[10]="dhccnew";
        int iNum=0;
        int iNum1=0;
        struct stat  tFileStat;


        if( stat( psInFile, &tFileStat ) )
        {
               return -1;
        }
        iLen=tFileStat.st_size;

        psBuf=NULL;
        psBuf=(char*)malloc(iLen*sizeof(char)+1);
        if(psBuf == NULL)
        {
                return -1;
        }
        memset(psBuf,0x00,iLen*sizeof(char)+1);
        fp1=fopen(psInFile, "rb");
        if (fp1 == NULL)
        {
                free(psBuf);
                psBuf=NULL;
                return -1;
        }


        fread(psBuf,1,iLen,fp1);
        fclose(fp1);
        psBuf[iLen]='\0';

        for(i=0; i<iLen; i++)
        {
                psBuf[i]=psBuf[i]^sPwd[iNum1 >= iNum ?iNum1=0:iNum1++];
        }


        fp2=fopen(psOutFile,"wb");
        if (fp2 == NULL)
        {
                free(psBuf);
                psBuf=NULL;
                return -1;
        }

        fwrite(psBuf,iLen,1,fp2);
        fclose(fp2);


        free(psBuf);
        psBuf=NULL;

        return 0;
}


