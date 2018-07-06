#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>     
#include <errno.h>      
#include "pub_log.h"


int main(int argc ,char **argv)
{
	int i=0;
	int buflen = 0;
	int outlen = 0;
	int dec = 0;
	char input_default[1024 + 8];
	char key[32];
	char output_default[1024 * 2 + 16];
	char *input = NULL;
	char *output = NULL;

	memset(key, 0x0, sizeof(key));

	if (argc == 4 && strcmp(argv[3], "-d") == 0)
	{
		dec = 1;
		strncpy(key, argv[2], sizeof(key)-1);
	}
	else if (argc == 3)
	{
		strncpy(key, argv[2], sizeof(key)-1);
	}
	else if (argc != 2)
	{
		printf("USAGE: swinfoenc   INPUTDATA   [key]   [-d]\n");
		return 0;
	}

	buflen=strlen(argv[1]);
	if (buflen > 1024)
	{
		input = malloc(buflen + 8);	
		if (input == NULL)
		{
			printf("malloc input [%d] error\n", buflen + 8);
			return -1;
		}
		output = malloc(2 * buflen + 16);	
		if (input == NULL)
		{
			printf("malloc output[%d] error\n", 2 * buflen + 16);
			return -1;
		}
	}
	else
	{
		input = input_default;
		output = output_default;
	}

	memset(input, 0x0, buflen + 8);
	memset(output, 0x0, 2 * buflen + 16);
	strcpy(input, argv[1]); 

	if (dec)
	{
		pub_des3_dec(input , output, buflen, key);
		printf("AFTER DEC:%s\n", output);
	}
	else
	{
		pub_des3_enc(input , output, buflen, key);
		printf("AFTER ENC:%s\n", output);
	}


	if (input != input_default)
	{
		free(input);
		free(output);
	}
	return 0;
}
