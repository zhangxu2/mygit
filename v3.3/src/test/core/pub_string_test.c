#include <stdio.h>
#include "pub_string.h"
int main()
{
        /* ѹ���ո� */
        /*
        char str[50] = "  pub_str_zipspace test";
        printf("%s\n", str);
        pub_str_zipspace(str);
        printf("%s\n", str);  
        */
        
        /* ѹ��09 0a�ַ� */
        /*
        char str[50];
        sprintf(str, "pub_str_zip%ctab%center", 0x09, 0x0a);
        printf("%s\n", str);  
        pub_str_zip0a09(str, 1);
        printf("%s\n", str);  
        */

        /* �ܴ���\xNNת���strcpy�汾 */
        /*
        char str1[50] = "pub_str_cpyext\\x38";
        char str2[50];
        printf("%s\n", str1);
        pub_str_cpyext(str2, str1); 
        printf("%s\n", str2);
        */

        /* ���ַ������ҵ�ƥ����ַ�������ƥ���ָ�� */
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

        /* ���ַ���src�г���Ϊn�Ĵ�д�ַ���תСд�ַ� */
        /*
        unsigned char str1[50] = "pub_STR_strlow";
        unsigned char str2[50];
        printf("%s\n", str1);
        pub_str_strlow(str2, str1, strlen(str1));
        printf("%s\n", str2);
        */

        /* ������Ϊn���ַ���������Ŀ���ַ��� */
        /*
        unsigned char str1[50] = "pub_str_cpystrn";
        unsigned char str2[50];
        pub_str_cpystrn(str2, str1, 3);
        printf("%s\n", str2);
        */

        /* ���ִ�Сд�Ƚ������ַ����Ƿ���ͬ */
        /*
        unsigned char str1[50] = "pub_str_strcasecmp";
        unsigned char str2[50] = "pub_STR_strcasecmp";
        int ret;
        ret = pub_str_strcasecmp(str2, str1);
        printf("%d\n", ret);
        */

        /* ���ִ�Сд�Ƚ�ָ�����ȵ������ַ����Ƿ���ͬ */
        /*
        unsigned char str1[50] = "pub_str_strncaserrr";
        unsigned char str2[50] = "pub_STR_strncasecmp";
        int ret;
        ret = pub_str_strncasecmp(str2, str1, strlen(str1));
        printf("%d\n", ret);
        */

        /* ��ָ����С���ַ������Ƿ������ַ��� */
        
        unsigned char str1[50] = "version:v2.1.2";
        unsigned char str2[40] = ":";
        unsigned char *p;
        p = pub_str_strnstr(str1, str2, strlen(str1));
        printf("%s\n", p);
        

        /* ��һ���ַ������Ƿ���ָ����С���ַ��� */
        /*
        unsigned char str1[50] = "pub_str_strstrn";
        unsigned char str2[50] = "str";
        unsigned char *p;
        p = pub_str_strstrn(str1, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */
        
        /* ��һ���ַ������Ƿ���ָ����С���ַ����������ִ�Сд */
        /*
        unsigned char str1[50] = "pub_str_strcasestrn";
        unsigned char str2[50] = "CASE";
        unsigned char *p;
        p = pub_str_strcasestrn(str1, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */

        /* ��һ���ַ������Ƿ���ָ����С���ַ����������ִ�Сд */
        /*
        unsigned char str1[50] = "pub_str_strlcasestrn";
        unsigned char str2[50] = "CASE";
        unsigned char *p;
        p = pub_str_strlcasestrn(str1, str1+16, str2, strlen(str2) - 1);
        printf("%s\n", p);
        */

        /* �Ӻ���ǰ�Ƚ������ַ����Ƿ���ͬ */
        /*
        unsigned char str1[50] = "pub_str_rstrncmp";
        unsigned char str2[50] = "ddddd_rstrncmp";
        int ret;
        ret = pub_str_rstrncmp(str1, str2, strlen(str2));
        printf("%d\n", ret);
        */

        /* �Ӻ���ǰ�Ƚ������ַ����Ƿ���ͬ�������ִ�Сд */
        /*
        unsigned char str1[50] = "pub_str_rstrncasecmp";
        unsigned char str2[50] = "QUB";
        int ret;
        ret = pub_str_rstrncasecmp(str1, str2, strlen(str2));
        printf("%d\n", ret);
        */

        /* �Ƚ�����ָ�����ȵ��ڴ��Ƿ���ͬ */
        /*
        unsigned char str1[50] = "pub_str_memn2cmp";
        unsigned char str2[50] = "pub_str_memn2cmp";
        int ret;
        ret = pub_str_memn2cmp(str1, str2, strlen(str1), strlen(str2));
        printf("%d\n", ret);
        */

        /* ָ�����ȵ��ַ���ת�������� */
        /*
        unsigned char str1[50] = "12345678";
        int ret;
        ret = pub_str_atoi(str1, strlen(str1));
        printf("%d\n", ret);
        */
        
        /* ָ�����ȵ��ַ���ת���� ssize_t�������� */
        /*
        unsigned char str1[50] = "12345678";
        ssize_t ret;
        ret = pub_str_atosz(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* ָ�����ȵ��ַ���ת���� time_t�������� */
        /*
        unsigned char str1[50] = "12345678";
        time_t ret;
        ret = pub_str_atotm(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* ָ�����ȵ��ַ���ת���� off_t(long int)�������� */
        /*
        unsigned char str1[50] = "12345678";
        off_t ret;
        ret = pub_str_atoof(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* ָ�����ȵ��ַ�����ʾ��ʮ����������ת����ʮ�������� */
        /*
        unsigned char str1[50] = "2a";
        int ret;
        ret = pub_str_hextoi(str1, strlen(str1));
        printf("%ld\n", ret);
        */

        /* ��ָ�����ȵ����ִ�ת����16���Ƶ��ַ��� */\
        /*
        unsigned char str1[50] = "45";
        unsigned char str2[50];
        char *p;
        memset(str2, 0, sizeof(str2));
        p = pub_str_hex_dump(str2, str1, strlen(str1));
        printf("%s\n", str2);
	*/
        /* ȥ���ַ�����˵Ŀո� */
        /*
        unsigned char str1[50] = "          pub_str_ltrim";
        pub_str_ltrim(str1);
        printf("%s\n", str1);
        */

        /* ȥ���ַ����Ҷ˵Ŀո� */
        /*
        unsigned char str1[50] = "pub_str_rtrim test   ";
        pub_str_rtrim(str1);
        printf("%s\n", str1);
        */

        /* ȥ���ַ������˵Ŀո� */
        /*
        unsigned char str1[50] = " pub_str_trim   ";
        pub_str_trim(str1);
        printf("%s\n", str1);
        */

        /* ��str1�е��ַ���str2�滻��str3 */
        /*
        unsigned char str1[50] = "pub_str_replace";
        pub_str_replace(str1, "str", "new");
        printf("%s\n", str1);
        */

        /* ��str1�е�str2�ַ���ȫ���滻��str3 */
        /*
        unsigned char str1[50] = "pub_str_replaceall_str";
        pub_str_replaceall(str1, "str", "new");
        printf("%s\n", str1);
        */

        /* ��ȡ��start��ʼ����end�������ַ�����Ŀ���ַ����� */
        /*
        unsigned char str1[50] = "pub_str_substring";
        unsigned char str2[50];
        pub_str_substring(str2, str1, 4, 100);
        printf("%s\n", str2);
        */
        
        /* �����ַ���������Ϊindex���ַ� */
        /*
        unsigned char str1[50] = "pub_str_charat";
        unsigned char c;
        c = pub_str_charat(str1, 3);
        printf("%c\n", c);
        */

        /* �ҳ�str2�ַ�����str1�ַ����е�һ�γ��ֵ�λ�� */
        /*
        unsigned char str1[50] = "pub_str_indexof";
        unsigned char str2[50] = "in";
        int pos;
        pos = pub_str_indexof(str1, str2);
        printf("%d\n", pos);
        */

        /* �ҳ�str2�ַ�����str1�ַ��������һ�γ��ֵ�λ�� */
        /*
        unsigned char str1[50] = "pub_str_lastindexof";
        unsigned char str2[50] = "s";
        int pos;
        pos = pub_str_lastindexof(str1, str2);
        printf("%d\n", pos);
        */

        /* �Ѹ������͵����ݸ�ʽ�������buf�����ĳ���Ϊ65536 */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_sprintf(str1, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */

        /* �Ѹ������͵����ݸ�ʽ�������ָ�����ȵ�buf */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_snprintf(str1, 10, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */

        /* �Ѹ������͵����ݸ�ʽ�������ָ�����ȵ�buf */
        /*
        unsigned char str1[50];
        unsigned char str2[50] = "meinv";
        memset(str1, 0, sizeof(str1));
        pub_str_slprintf(str1, str1+10, "nihao %s bye %s", str2, str2);
        printf("%s\n", str1);
        */
        
        /* ����Ƿ��а������ */
        /*
        unsigned char str1[50] = "���";
        int ret;
        ret = pub_str_chkhalf(str1, 4);
        if(ret == SW_OK)
            printf("OK\n");
        else
            printf("NO\n");
        */
        
        /* ��������ַ� */ 
        /*
        unsigned char str1[50] = "���%";
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
