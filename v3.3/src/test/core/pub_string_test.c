#include <stdio.h>
#include "pub_string.h"
int main()
{
        /* 压缩空格 */
        /*
        char str[50] = "  pub_str_zipspace test";
        printf("%s\n", str);
        pub_str_zipspace(str);
        printf("%s\n", str);  
        */
        
        /* 压缩09 0a字符 */
        /*
        char str[50];
        sprintf(str, "pub_str_zip%ctab%center", 0x09, 0x0a);
        printf("%s\n", str);  
        pub_str_zip0a09(str, 1);
        printf("%s\n", str);  
        */

        /* 能处理\xNN转义的strcpy版本 */
        /*
        char str1[50] = "pub_str_cpyext\\x38";
        char str2[50];
        printf("%s\n", str1);
        pub_str_cpyext(str2, str1); 
        printf("%s\n", str2);
        */

        /* 在字符串中找到匹配的字符，返回匹配的指针 */
        /*
        int iRet;
	char sBuf[50];
	unsigned char *sVersion;
	unsigned char *q;
	FILE *fp = fopen("123", "rb");
	
	memset(sBuf, 0x0, sizeof(sBuf));
	while(fgets(sBuf, sizeof(sBuf) - 1, fp) != NULL)
	{
		sBuf[strlen(sBuf) - 1] = '\0';
		iRet = pub_str_zipspace(sBuf);
		if (iRet)
		{
			syslog(LOG_ERR, "zipspace err");
			return NULL;
		}
		else
		{
			if (strncmp(sBuf, "version", 7))
			{
				continue;
			}
			else
			{
        			q = sBuf + 8;
        			sVersion = pub_str_strlchr(sBuf, q, ':');
        			printf("%s\n", sVersion+1);
			}
		}
	}
        */

        /* 将字符串src中长度为n的大写字符串转小写字符 */
        /*
        unsigned char str1[50] = "pub_STR_strlow";
        unsigned char str2[50];
        printf("%s\n", str1);
        pub_str_strlow(str2, str1, strlen(str1));
        printf("%s\n", str2);
        */

        /* 将长度为n的字符串拷贝到目的字符串 */
        /*
        unsigned char str1[50] = "pub_str_cpystrn";
        unsigned char str2[50];
        pub_str_cpystrn(str2, str1, 3);
        printf("%s\n", str2);
        */

        /* 不分大小写比较两个字符串是否相同 */
        /*
        unsigned char str1[50] = "pub_str_strcasecmp";
        unsigned char str2[50] = "pub_STR_strcasecmp";
        int ret;
        ret = pub_str_strcasecmp(str2, str1);
        printf("%d\n", ret);
        */

        /* 不分大小写比较指定长度的两个字符串是否相同 */
        /*
        unsigned char str1[50] = "pub_str_strncaserrr";
        unsigned char str2[50] = "pub_STR_strncasecmp";
        int ret;
        ret = pub_str_strncasecmp(str2, str1, strlen(str1));
        printf("%d\n", ret);
        */

        /* 在指定大小的字符串中是否有子字符串 */
        
        unsigned char str1[50] = "version:v2.1.2";
        unsigned char str2[40] = ":";
        unsigned char *p;
        p = pub_str_strnstr(str1, str2, strlen(str1));
        printf("%s\n", p);
        

        /* 在一个字符串中是否有指定大小的字符串 */
        /*
        unsigned char str1[50] = "pub_str_strstrn";
        unsigned char str2[50] = "str";
        unsigned char *p;
        p = pub_str_strstrn(str1, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */
        
        /* 在一个字符串中是否有指定大小的字符串，不区分大小写 */
        /*
        unsigned char str1[50] = "pub_str_strcasestrn";
        unsigned char str2[50] = "CASE";
        unsigned char *p;
        p = pub_str_strcasestrn(str1, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */

        /* 在一个字符串中是否有指定大小的字符串，不区分大小写 */
        /*
        unsigned char str1[50] = "pub_str_strlcasestrn";
        unsigned char str2[50] = "CASE";
        unsigned char *p;
        p = pub_str_strlcasestrn(str1, str1+16, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */

        /* 从后往前比较两个字符串是否相同 */
        /*
        unsigned char str1[50] = "pub_str_rstrncmp";
        unsigned char str2[50] = "ddddd_rstrncmp";
        int ret;
        ret = pub_str_rstrncmp(str1, str2, strlen(str2));
        printf("%d\n", ret);
        */

        /* 从后往前比较两个字符串是否相同，不区分大小写 */
        /*
        unsigned char str1[50] = "pub_str_rstrncasecmp";
        unsigned char str2[50] = "QUB";
        int ret;
        ret = pub_str_rstrncasecmp(str1, str2, strlen(str2));
        printf("%d\n", ret);
        */

        /* 比较两个指定长度的内存是否相同 */
        /*
        unsigned char str1[50] = "pub_str_memn2cmp";
        unsigned char str2[50] = "pub_str_memn2cmp";
        int ret;
        ret = pub_str_memn2cmp(str1, str2, strlen(str1), strlen(str2));
        printf("%d\n", ret);
        */

        /* 指定长度的字符串转换成数字 */
        /*
        unsigned char str1[50] = "12345678";
        int ret;
        ret = pub_str_atoi(str1, strlen(str1));
        printf("%d\n", ret);
        */
        
        /* 指定长度的字符串转换成 ssize_t类型数字 */
        /*
        unsigned char str1[50] = "12345678";
        ssize_t ret;
        ret = pub_str_atosz(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* 指定长度的字符串转换成 time_t类型数字 */
        /*
        unsigned char str1[50] = "12345678";
        time_t ret;
        ret = pub_str_atotm(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* 指定长度的字符串转换成 off_t(long int)类型数字 */
        /*
        unsigned char str1[50] = "12345678";
        off_t ret;
        ret = pub_str_atoof(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* 指定长度的字符串表示的十六进制数字转换成十进制数字 */
        /*
        unsigned char str1[50] = "2a";
        int ret;
        ret = pub_str_hextoi(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* 把指定长度的数字串转换成16进制的字符串 */\
        /*
        unsigned char str1[50] = "45";
        unsigned char str2[50];
        char *p;
        memset(str2, 0, sizeof(str2));
        p = pub_str_hex_dump(str2, str1, strlen(str1));
        printf("%s\n", str2);
	*/
        /* 去除字符串左端的空格 */
        /*
        unsigned char str1[50] = "          pub_str_ltrim";
        pub_str_ltrim(str1);
        printf("%s\n", str1);
        */

        /* 去除字符串右端的空格 */
        /*
        unsigned char str1[50] = "pub_str_rtrim test   ";
        pub_str_rtrim(str1);
        printf("%s\n", str1);
        */

        /* 去除字符串两端的空格 */
        /*
        unsigned char str1[50] = " pub_str_trim   ";
        pub_str_trim(str1);
        printf("%s\n", str1);
        */

        /* 将str1中的字符串str2替换成str3 */
        /*
        unsigned char str1[50] = "pub_str_replace";
        pub_str_replace(str1, "str", "new");
        printf("%s\n", str1);
        */

        /* 将str1中的str2字符串全部替换成str3 */
        /*
        unsigned char str1[50] = "pub_str_replaceall_str";
        pub_str_replaceall(str1, "str", "new");
        printf("%s\n", str1);
        */

        /* 截取从start开始，到end结束的字符串到目的字符串中 */
        /*
        unsigned char str1[50] = "pub_str_substring";
        unsigned char str2[50];
        pub_str_substring(str2, str1, 4, 100);
        printf("%s\n", str2);
        */
        
        /* 返回字符串中索引为index的字符 */
        /*
        unsigned char str1[50] = "pub_str_charat";
        unsigned char c;
        c = pub_str_charat(str1, 3);
        printf("%c\n", c);
        */

        /* 找出str2字符串在str1字符串中第一次出现的位置 */
        /*
        unsigned char str1[50] = "pub_str_indexof";
        unsigned char str2[50] = "in";
        int pos;
        pos = pub_str_indexof(str1, str2);
        printf("%d\n", pos);
        */

        /* 找出str2字符串在str1字符串中最后一次出现的位置 */
        /*
        unsigned char str1[50] = "pub_str_lastindexof";
        unsigned char str2[50] = "s";
        int pos;
        pos = pub_str_lastindexof(str1, str2);
        printf("%d\n", pos);
        */

        /* 把各种类型的数据格式化输出到buf，最大的长度为65536 */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_sprintf(str1, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */

        /* 把各种类型的数据格式化输出到指定长度的buf */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_snprintf(str1, 10, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */

        /* 把各种类型的数据格式化输出到指定长度的buf */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_slprintf(str1, str1+10, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */
        
        /* 检查是否有半个汉字 */
        /*
        unsigned char str1[50] = "你好";
        int ret;
        ret = pub_str_chkhalf(str1, 4);
        if(ret == SW_OK)
            printf("OK\n");
        else
            printf("NO\n");
        */
        
        /* 检查中文字符 */ 
        /*
        unsigned char str1[50] = "你好%";
        int ret;
        printf("%s\n", str1);
        ret = pub_str_chkchinese(str1);
        if(ret == SW_OK)
            printf("OK\n");
        else
            printf("NO\n");
        printf("%s\n", str1);
        */

        return 0;
}
