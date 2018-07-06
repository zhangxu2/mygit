/*********************************************************************
 *** 版 本 号: v2.0
 *** 程序作者: liteng
 *** 生成日期: 2012-4-19
 *** 所属模块: 
 *** 程序名称: 
 *** 程序作用:
 *** 函数列表:
 ***		pub_str_zipspace	压缩空格
 ***		pub_str_zip0a09		压缩0a、09字符
 ***		pub_str_cpyext		能处理\xNN转义的strcpy版本
 ***		pub_str_strlchr         在字符串中找到匹配的字符，返回匹配的指针
 ***		pub_str_strlow          将字符串src中长度为n的大写字符串转小写字符
 ***		pub_str_cpystrn         将长度为n的字符串拷贝到目的字符串
 ***		pub_str_strcasecmp      不分大小写比较两个字符串是否相同
 ***		pub_str_strncasecmp     不分大小写比较指定长度的两个字符串是否相同 
 ***		pub_str_strnstr         在指定大小的字符串中是否有子字符串 
 ***		pub_str_strstrn         在一个字符串中是否有子指定大小的字符串 
 ***		pub_str_strcasestrn     在一个字符串中是否有子指定大小的字符串，不区分大小写
 ***		pub_str_strlcasestrn    在一个字符串中是否有子指定大小的字符串，不区分大小写
 ***		pub_str_rstrncmp        从后往前比较两个字符串是否相同，返回相同的位置
 ***		pub_str_rstrncasecmp    从后往前比较两个字符串是否相同，返回相同的位置，不区分大小写
 ***		pub_str_memn2cmp        比较两个指定长度的内存是否相同，也比较长的内存是否包含短的内存
 ***		pub_str_atoi            指定长度的字符串转换成数字
 ***		pub_str_atosz           指定长度的字符串转换成 ssize_t类型数
 ***		pub_str_atotm           指定长度的字符串转换成time_t类型数字
 ***		pub_str_atoof           指定长度的字符串转换成off_t类型数字
 ***		pub_str_hextoi          指定长度的字符串转换成十六进制数字
 ***		pub_str_hex_dump        把指定长度的数字串转换成16进制的字符串
 ***		pub_str_ltrim           去除字符串左端的空格
 ***		pub_str_rtrim           去除字符串右端的空格
 ***		pub_str_trim            去除字符串两端的空格
 ***		pub_str_replace         将str1中的字符串str2替换成str3
 ***		pub_str_replaceall      将str1中的str2字符串全部替换成str3
 ***		pub_str_substring       截取从start开始，到end结束的字符串到目的字符串中
 ***		pub_str_charat          返回字符串中索引为index的字符
 ***		pub_str_lastindexof     找出str2字符串在str1字符串中最后一次出现的位置
 ***		pub_str_indexof         找出str2字符串在str1字符串中第一次出现的位置
 ***		pub_str_sprintf 	打印格式字符串
 ***		pub_str_checkincl	检查是否包含字符串
 *** 使用注意:
 *** 修改记录:
 *** 	修改作者: yangzhao
 *** 	修改时间: 2013-3-19
 *** 	修改内容: pub_str_cpystrn原函数复制的字符数为n-1，将while(--n)改为(n--)
 ***		  pub_str_charat函数while循环多余，直接返回p[index]
 ***		  pub_str_hex_dump函数返回值做了修改
 ***		  部分注释修改
 ********************************************************************/
#include "pub_string.h"
#include "pub_code.h"
#include "pub_log.h"
#include <ctype.h>

static sw_char_t *pub_str_sprintf_num(sw_char_t *buf, sw_char_t *last, uint64_t ui64, 
	sw_char_t zero, unsigned int hexadecimal, unsigned int width)
{
	sw_char_t         *p, temp[SW_INT64_LEN + 1];
	/*
	* we need temp[PUB_INT64_LEN] only,
	* but icc issues the warning
	*/
	size_t          len;
	uint32_t        ui32;
	static sw_char_t   hex[] = "0123456789abcdef";
	static sw_char_t   HEX[] = "0123456789ABCDEF";

	p = temp + SW_INT64_LEN;

	if (hexadecimal == 0) 
	{
		if (ui64 <= SW_MAX_INT32_VALUE) 
		{
			/*
			* To divide 64-bit numbers and to find remainders
			* on the x86 platform gcc and icc call the libc functions
			* [u]divdi3() and [u]moddi3(), they call another function
			* in its turn.  On FreeBSD it is the qdivrem() function,
			* its source code is about 170 lines of the code.
			* The glibc counterpart is about 150 lines of the code.
			*
			* For 32-bit numbers and some divisors gcc and icc use
			* a inlined multiplication and shifts.  For example,
			* unsigned "i32 / 10" is compiled to
			*
			*     (i32 * 0xCCCCCCCD) >> 35
			*/
		
			ui32 = (uint32_t) ui64;
		
			do 
			{
				*--p = (sw_char_t) (ui32 % 10 + '0');
			} while (ui32 /= 10);
		
		} 
		else 
		{
			do 
			{
				*--p = (sw_char_t) (ui64 % 10 + '0');
			} while (ui64 /= 10);
		}
	
	} 
	else if (hexadecimal == 1) 
	{
		do 
		{
			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = hex[(uint32_t) (ui64 & 0xf)];
		} while (ui64 >>= 4);
	
	} 
	else 
	{ 
		/* hexadecimal == 2 */
		do 
		{
		
			/* the "(uint32_t)" cast disables the BCC's warning */
			*--p = HEX[(uint32_t) (ui64 & 0xf)];
		
		} while (ui64 >>= 4);
	}

	/* zero or space padding */
	
	len = (temp + SW_INT64_LEN) - p;

	while (len++ < width && buf < last) 
	{
		*buf++ = zero;
	}

	/* number safe copy */
	len = (temp + SW_INT64_LEN) - p;
	
	if (buf + len > last) 
	{
		len = last - buf;
	}
	
	return (sw_char_t*)memcpy(buf, p, len);
}

/******************************************************************************
 *** 函数名称: pub_str_zipspace
 *** 函数功能: 压缩空格
 *** 函数作者: liteng
 *** 生成日期: 2012-4-19 15:28
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: psBuf	数据缓冲区
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
 ******************************************************************************/
 /*zip_space*/
int pub_str_zipspace(sw_char_t *psBuf)
{
	int i;
	int j;
	 
	if(psBuf == NULL||strlen((sw_char_t*)psBuf) == 0)
	{
		return -1;
	}
 
	i = 0;
	j = 0;
	while(psBuf[i] != '\0')
	{
		if(psBuf[i] != ' ')
		{
			if(i != j)
			{
				psBuf[j]=psBuf[i];
			}
			i++;
			j++;
		}
		else
		{
			i++;
		}
	}

	while(j < i)
	{
		psBuf[j] = '\0';
		j++;
	}

	return(0);
}
/**
函数名:pub_str_ziphlspace
功能  :去掉前导空格和结尾空格
参数  :
       字符串
返回值:
       0/-1
**/
sw_int_t pub_str_ziphlspace(sw_char_t *str)
{
	sw_int32_t	begin_flag = 0;
	sw_int32_t	i = 0;
	sw_int32_t	j = 0;
	
	if (str == NULL)
	{
		return SW_ERROR;
	}
	
	while(str[i]!='\0')
	{
		if (str[i] == ' ' || str[i] == '\t')
		{
			if(!begin_flag)
			{
				i++;
				continue;
			}
			else
			{
				str[j++] = str[i++];
				continue;
			}
		}
		else
		{
			begin_flag = 1;
			str[j++] = str[i++];
			continue;
		}
	}
	
	str[j] = '\0';
	j--;
	while (j > 0)
	{
		if (str[j] == ' ')
		{
			str[j] = '\0';
		}
		else
		{
			break;
		}
		
		j--;
	}

	return SW_OK;
}


/******************************************************************************
 *** 函数名称: pub_str_zip0a09
 *** 函数功能: 压缩0a、09字符
 *** 函数作者: liteng
 *** 生成日期: 2012-4-19 15:31
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: psBuf	数据缓冲区
 ***    参数2: iBegin	起始索引
 *** 输出参数: 
 *** 	参数1: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
 ******************************************************************************/
/*zip_0a09*/
 int pub_str_zip0a09(char *psBuf, int iBegin)
 {
	 int i;
	 int j;

	 if(iBegin < 0)
	 {
	 	return -9;
	 }

	 i = iBegin;
	 j = iBegin;
	 while(psBuf[i] != '\0')
	 {
		if(psBuf[i] != 0x09&&psBuf[i] != 0x0a)
		{
			if(i != j)
			{
				psBuf[j] = psBuf[i];
			}
			i++;
			j++;
		}
		else
		{
			i++;
		}
	 }

	 if(j < i)
	 {
		psBuf[j] = '\0';
	 }

	 return(j);
}

/******************************************************************************
 *** 函数名称: pub_str_cpyext
 *** 函数功能: 能处理\xNN转义的strcpy版本
 *** 函数作者: liteng
 *** 生成日期: 2012-4-19 15:57
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: psBuf2	输入数据缓冲区
 *** 输出参数: 
 *** 	参数1: psBuf1   输出数据缓冲区
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
 ******************************************************************************/
 /*strcpy_ext*/
int pub_str_cpyext(sw_char_t *psBuf1, sw_char_t *psBuf2)
{
	int i;
	int j;
	char sTmp[2];

	i = 0;
	j = 0;
	memset(sTmp,0x00,sizeof(sTmp));
	while(psBuf2[i] != '\0')
	{
		if(psBuf2[i] == '\\'&&psBuf2[i+1] == 'x')
		{
			pub_code_asctobcd(psBuf2 + i + 2, sTmp, 2);
			psBuf1[j] = sTmp[0];
			i += 4;
			j++;
		}
		else
		{
			psBuf1[j] = psBuf2[i];
			i++;
			j++;
		}
	}
	psBuf1[j] = '\0';
	return(j);
}

/******************************************************************************
 *** 函数名称: pub_str_strlchr
 *** 函数功能: 在字符串中找到匹配的字符，返回匹配的指针
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: src	源字符串
 ***    参数2: last	src界定字符
 ***    参数3: c	查找字符
 *** 输出参数: 
 *** 返 回 值: 目标字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t * pub_str_strlchr(sw_char_t *src, sw_char_t *last, sw_char_t c)
{
	while(src < last)
	{
		if(*src == c)
		{
			return src;
		}
		src++;
	}
	
	return NULL;
}

/******************************************************************************
 *** 函数名称: pub_str_strlow
 *** 函数功能: 将字符串src中长度为n的大写字符串转小写字符
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	要转换的字符长度
 *** 输出参数: 
  *** 	参数1: dst	目的字符串
 *** 返 回 值: 0 成功
 *** 注意事项: 
 ******************************************************************************/
int pub_str_strlow(sw_char_t *dst, const sw_char_t *src, size_t n)
{    
	while (n) 
	{        
		*dst = pub_str_tolower(*src);
		dst++;        
		src++;        
		n--;    
	}
	
	return 0;
}

/******************************************************************************
 *** 函数名称: pub_str_cpystrn
 *** 函数功能: 将长度为n的字符串拷贝到目的字符串
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	要转换的字符长度
 *** 输出参数: 
  *** 	参数1: dst	目的字符串
 *** 返 回 值: 0 成功
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_cpystrn(sw_char_t *dst, const sw_char_t *src, size_t n)
{    
	if (n == 0) 
	{        
		return dst;    
	}    
	
	while (n--) 
	{        
		*dst = *src;        
		if (*dst == '\0') 
		{            
			return dst;        
		}        
		dst++;        
		src++;    
	}    
	
	*dst = '\0'; 
	   
	return dst;
}

/******************************************************************************
 *** 函数名称: pub_str_strcasecmp
 *** 函数功能: 不分大小写比较两个字符串是否相同
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	字符串1
 ***    参数2: s2	字符串2
 *** 输出参数: 
 *** 返 回 值: >0 大于 <0 小于 =0 等于
 *** 注意事项: 
 ******************************************************************************/
int pub_str_strcasecmp(sw_char_t *s1, sw_char_t *s2)
{
	unsigned int	c1, c2;
	
	for ( ;; ) 
	{
		c1 = (unsigned int) *s1++;
		c2 = (unsigned int) *s2++;
		
		c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
	
		if (c1 == c2) 
		{
		
			if (c1) 
			{
				continue;
			}
		
			return 0;
		}
		
		return c1 - c2;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_strncasecmp
 *** 函数功能: 不分大小写比较指定长度的两个字符串是否相同
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	字符串1
 ***    参数2: s2	字符串2
 ***    参数3: n	长度
 *** 输出参数: 
 *** 返 回 值: >0 大于 <0 小于 =0 等于
 *** 注意事项: 
 ******************************************************************************/
int pub_str_strncasecmp(sw_char_t *s1, sw_char_t *s2, size_t n)
{
	unsigned int	c1, c2;
	
	while (n) 
	{
		c1 = (unsigned int) *s1++;
		c2 = (unsigned int) *s2++;
	
		c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
	
		if (c1 == c2) 
		{
		
			if (c1) 
			{
				n--;
				continue;
			}
		
			return 0;
		}
	
		return c1 - c2;
	}
	
	return 0;
}

/******************************************************************************
 *** 函数名称: pub_str_strnstr
 *** 函数功能: 在指定大小的字符串中是否有子字符串
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	字符串1
 ***    参数2: s2	字符串2
 ***    参数3: len	字符串s1的长度
 *** 输出参数: 
 *** 返 回 值: dst	匹配字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_strnstr(sw_char_t *s1, char *s2, size_t len)
{
	sw_char_t  c1, c2;
	size_t  n;
	
	c2 = *(sw_char_t *) s2++;
	
	n = pub_str_strlen(s2);
	
	do 
	{
		do 
		{
			if (len-- == 0) 
			{
				return NULL;
			}
		
			c1 = *s1++;
		
			if (c1 == 0) 
			{
				return NULL;
			}
		
		} while (c1 != c2);
		
		if (n > len) 
		{
			return NULL;
		}
	
	} while (pub_str_strncmp(s1, (sw_char_t *) s2, n) != 0);
	
	return --s1;
}

/******************************************************************************
 *** 函数名称: pub_str_strstrn
 *** 函数功能: 在一个字符串中是否有指定大小的字符串
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	比较字符串1
 ***    参数2: s2	比较字符串2
 ***    参数3: n	字符串s2的长度，s2长度减1
 *** 输出参数: 
 *** 返 回 值: dst	匹配字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_strstrn(sw_char_t *s1, char *s2, size_t n)
{
	sw_char_t  c1, c2;
	
	c2 = *(sw_char_t *) s2++;
	
	do 
	{
		do 
		{
			c1 = *s1++;
			
			if (c1 == 0) 
			{
				return NULL;
			}
		
		} while (c1 != c2);
	
	} while (pub_str_strncmp(s1, (sw_char_t *) s2, n) != 0);
	
	return --s1;
}

/******************************************************************************
 *** 函数名称: pub_str_strcasestrn
 *** 函数功能: 在一个字符串中是否有指定大小的字符串，不区分大小写
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	比较字符串1
 ***    参数2: s2	比较字符串2
 ***    参数3: n	字符串s2的长度，s2长度减1
 *** 输出参数: 
 *** 返 回 值: dst	匹配字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_strcasestrn(sw_char_t *s1, char *s2, size_t n)
{
	unsigned int	c1, c2;
	
	c2 = (unsigned int) *s2++;
	c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;

	do 
	{
		do 
		{
			c1 = (unsigned int) *s1++;
			
			if (c1 == 0) 
			{
				return NULL;
			}
			
			c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		
		} while (c1 != c2);
	
	} while (pub_str_strncasecmp(s1, (sw_char_t *) s2, n) != 0);
	
	return --s1;
}

/******************************************************************************
 *** 函数名称: pub_str_strlcasestrn
 *** 函数功能: 在一个字符串中是否有指定大小的字符串，不区分大小写
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	字符串1
 ***    参数2: last	字符串定界符
 ***    参数3: s2	字符串2
 ***    参数4: n	s2字符串长度，s2长度减1
 *** 输出参数: 
 *** 返 回 值: dst	匹配字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_strlcasestrn(sw_char_t *s1, sw_char_t *last, sw_char_t *s2, size_t n)
{
	unsigned int	c1, c2;
	
	c2 = (unsigned int) *s2++;
	c2 = (c2 >= 'A' && c2 <= 'Z') ? (c2 | 0x20) : c2;
	last -= n;

	do 
	{
		do 
		{
			if (s1 >= last) 
			{
				return NULL;
			}
		
			c1 = (unsigned int) *s1++;
			
			c1 = (c1 >= 'A' && c1 <= 'Z') ? (c1 | 0x20) : c1;
		
		} while (c1 != c2);
	
	} while (pub_str_strncasecmp(s1, s2, n) != 0);
	
	return --s1;
}

/******************************************************************************
 *** 函数名称: pub_str_rstrncmp
 *** 函数功能: 从后往前比较两个字符串是否相同，返回不同字符asc码之差
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	比较字符串1
 ***    参数2: s2	比较字符串2
 ***    参数3: n	比较长度
 *** 输出参数: 
 *** 返 回 值: 0：相同 非0：不相同
 *** 注意事项: 
 ******************************************************************************/
int pub_str_rstrncmp(sw_char_t *s1, sw_char_t *s2, size_t n)
{
	char    *p1 = NULL;
	char    *p2 = NULL;

	if (n == 0) 
	{
		return 0;
	}

	if (s1 == NULL && s2 == NULL)
	{
		return 0;
	}

	if (s1 == NULL || s2 == NULL)
	{
		return -1;
	}

	p1 = s1 + strlen(s1) - 1;
	p2 = s2 + strlen(s2) - 1;
	while (p1 >= s1 && p2 >= s2 && n > 0)
	{
		if (*p1 != *p2)
		{
			break;
		}
		p1--;
		p2--;
		n--;
	}

	if (n == 0)
	{
		return 0;
	}

	return -1;
}

/******************************************************************************
 *** 函数名称: pub_str_rstrncasecmp
 *** 函数功能: 从后往前比较两个字符串是否相同，返回不同字符asc码之差，不区分大小写
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	比较字符串1
 ***    参数2: s2	比较字符串2
 ***    参数3: n	比较长度
 *** 输出参数: 
 *** 返 回 值: 0：相同 非0：不相同
 *** 注意事项: 
 ******************************************************************************/
int pub_str_rstrncasecmp(sw_char_t *s1, sw_char_t *s2, size_t n)
{
	sw_char_t  c1, c2;
	
	if (n == 0) 
	{
		return 0;
	}

    	n--;

	for ( ;; ) 
	{
		c1 = s1[n];
		if (c1 >= 'a' && c1 <= 'z') 
		{
			c1 -= 'a' - 'A';
		}
	
		c2 = s2[n];
		if (c2 >= 'a' && c2 <= 'z') 
		{
			c2 -= 'a' - 'A';
		}
	
		if (c1 != c2) 
		{
			return c1 - c2;
		}
		
		if (n == 0) 
		{
			return 0;
		}
		
		n--;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_memn2cmp
 *** 函数功能: 比较两个指定长度的内存是否相同，也比较长的内存是否包含短的内存
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: s1	比较字符串1
 ***    参数2: s2	比较字符串2
 ***    参数3: n1	s1字符串的长度
 ***    参数4: n2	s2字符串的长度
 *** 输出参数: 
 *** 返 回 值: -1：s2内存大于或等于s1  1：s1内存大 
 *** 注意事项: 
 ******************************************************************************/
int pub_str_memn2cmp(sw_char_t *s1, sw_char_t *s2, size_t n1, size_t n2)
{
	size_t	n;
	int	m, z;
	
	if (n1 <= n2) 
	{
		n = n1;
		z = -1;
	} 
	else 
	{
		n = n2;
		z = 1;
	}
	
	m = memcmp(s1, s2, n);
	
	if (m || n1 == n2) 
	{
		return m;
	}
	
	return z;
}

/******************************************************************************
 *** 函数名称: pub_str_atoi
 *** 函数功能: 指定长度的字符串转换成数字
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	转换长度
 *** 输出参数: 
 *** 返 回 值: >=0成功 <0失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_atoi(sw_char_t *src, size_t n)
{
	int  value;
	
	if (n == 0) 
	{
		return -1;
	}

	for (value = 0; n--; src++) 
	{
		if (*src < '0' || *src > '9') 
		{
			return -1;
		}
		
		value = value * 10 + (*src - '0');
	}

	if (value < 0) 
	{
		return -1;
	} 
	else 
	{
		return value;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_atosz
 *** 函数功能: 指定长度的字符串转换成 ssize_t类型数
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	转换长度
 *** 输出参数: 
 *** 返 回 值: >=0成功 <0失败
 *** 注意事项: 
 ******************************************************************************/
ssize_t pub_str_atosz(sw_char_t *src, size_t n)
{
	ssize_t  value;
	
	if (n == 0) 
	{
		return -1;
	}

	for (value = 0; n--; src++) 
	{
		if (*src < '0' || *src > '9') 
		{
			return -1;
		}
		
		value = value * 10 + (*src - '0');
	}

	if (value < 0) 
	{
		return -1;
	} 
	else 
	{
		return value;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_atotm
 *** 函数功能: 指定长度的字符串转换成time_t类型数字
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	转换长度
 *** 输出参数: 
 *** 返 回 值: >=0成功 <0失败
 *** 注意事项: 
 ******************************************************************************/
time_t pub_str_atotm(sw_char_t *src, size_t n)
{
	time_t  value;
	
	if (n == 0) 
	{
		return -1;
	}
	
	for (value = 0; n--; src++) 
	{
		if (*src < '0' || *src > '9') 
		{
			return -1;
		}
	
		value = value * 10 + (*src - '0');
	}
	
	if (value < 0) 
	{
		return -1;
	} 
	else 
	{
		return value;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_atoof
 *** 函数功能: 指定长度的字符串转换成off_t类型数字
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	转换长度
 *** 输出参数: 
 *** 返 回 值: >=0成功 <0失败
 *** 注意事项: 
 ******************************************************************************/
off_t pub_str_atoof(sw_char_t *src, size_t n)
{
	off_t  value;
	
	if (n == 0) 
	{
		return -1;
	}

	for (value = 0; n--; src++) 
	{
		if (*src < '0' || *src > '9') 
		{
			return -1;
		}
		
		value = value * 10 + (*src - '0');
	}

	if (value < 0) 
	{
		return -1;
	} 
	else 
	{
		return value;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_hextoi
 *** 函数功能: 指定长度的字符串转换成十六进制数字
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: n	转换长度
 *** 输出参数: 
 *** 返 回 值: >=0成功 <0失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_hextoi(sw_char_t *src, size_t n)
{
	sw_char_t	c, ch;
	int	value;
	
	if (n == 0) 
	{
		return -1;
	}

	for (value = 0; n--; src++) 
	{
		ch = *src;
		
		if (ch >= '0' && ch <= '9') 
		{
			value = value * 16 + (ch - '0');
			continue;
		}
	
		c = (sw_char_t) (ch | 0x20);
		
		if (c >= 'a' && c <= 'f') 
		{
			value = value * 16 + (c - 'a' + 10);
			continue;
		}
		
		return -1;
	}
	
	if (value < 0) 
	{
		return -1;
	} 
	else 
	{
		return value;
	}
}

/******************************************************************************
 *** 函数名称: pub_str_hex_dump
 *** 函数功能: 把指定长度的数字串转换成16进制的字符串
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: 
 ***    参数2: 
 *** 输出参数: 
 ***    参数1: dst	处理后字符串
 *** 返 回 值: dst      处理后字符串
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_hex_dump(sw_char_t *dst, sw_char_t *src, size_t len)
{
	static sw_char_t  hex[] = "0123456789abcdef";

        char *p;
        p = dst;
	while (len--) 
	{
		*p++ = hex[*src >> 4];
		*p++ = hex[*src++ & 0xf];
	}
	
	return dst;
}

/******************************************************************************
 *** 函数名称: pub_str_ltrim
 *** 函数功能: 去除字符串左端的空格
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 *** 输出参数: 
 ***    参数1: src	处理后字符串
 *** 返 回 值: 0 成功 -1 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_ltrim(char *src)  
{  
	char *p;  
  	
  	p = src;  
	
	while(*p == ' ' || *p == '\t')
	{
		p++;
	} 
	 
  	strcpy(src,p);  
  	
  	return 0;
 }

/******************************************************************************
 *** 函数名称: pub_str_rtrim
 *** 函数功能: 去除字符串右端的空格
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 *** 输出参数: 
 ***    参数1: src	处理后字符串
 *** 返 回 值: 0 成功 -1 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_rtrim(char *src)  
{  
	int index;  
	
	index = pub_str_strlen(src) - 1;  
	while((src[index] == ' ' || src[index] == '\t') && index >= 0)
	{
		index--;
	}  
	src[index + 1] = '\0';  
	
	return 0;
}      

/******************************************************************************
 *** 函数名称: pub_str_trim
 *** 函数功能: 去除字符串两端的空格
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 *** 输出参数: 
 ***    参数1: src	处理后字符串
 *** 返 回 值: 0 成功 -1 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_trim(char *src)
{  
	pub_str_ltrim(src);  
	pub_str_rtrim(src); 
	
	return 0; 
}

/******************************************************************************
 *** 函数名称: pub_str_replace
 *** 函数功能: 将str1中的字符串str2替换成str3
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: str1	源字符串
 ***    参数2: str2	替换字符字符串
 ***    参数3: str3	新字符串
 *** 输出参数: 
 ***    参数1: str1	替换后新字符串
 *** 返 回 值: 0 成功 -1 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_replace(char *str1, const char *str2,const char *str3)
{
	int len=0, size=0;
	char *buf=NULL, *p=NULL;

	size = pub_str_strlen(str1) + 1;
	buf = calloc(1, size);
	if(NULL == buf)
	{
		return -1;
	}

	memset(buf, 0, size); 
	strcpy(buf, str1);
	
	pub_log_info("[%s][%d] str1:[%s] str2:[%s] str3:%s", __FILE__, __LINE__, str1, str2, str3);
	if((p = pub_str_strstr(str1, str2)) != NULL)
	{
		len = pub_str_strlen(str1) - pub_str_strlen(p);
		pub_log_info("[%s][%d] str1:[%d] p:%d len:%d",
			 __FILE__, __LINE__, pub_str_strlen(str1), strlen(p), len);
		str1[len] = '\0';
		strcat(str1,str3);
		strcat(str1,pub_str_strstr(buf,str2) + pub_str_strlen(str2));
	}
	
	free(buf), buf=NULL;
	
	return 0;
}

/******************************************************************************
 *** 函数名称: pub_str_replaceall
 *** 函数功能: 将str1中的str2字符串全部替换成str3
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: str1	源字符串
 ***    参数2: str2	替换字符字符串
 ***    参数3: str3	新字符串
 *** 输出参数: 
 ***    参数1: str1	替换后新字符串
 *** 返 回 值: 0 成功 -1 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_replaceall(char *str1, const char *str2, const char *str3)
{
	while(pub_str_strstr(str1, str2) != NULL)
	{
		pub_str_replace(str1,str2,str3);
	}
	
	return 0;
}

/******************************************************************************
 *** 函数名称: pub_str_substring
 *** 函数功能: 截取从start开始，到end结束的字符串到目的字符串中
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: start	起始位置
 ***    参数3: end	结束位置
 *** 输出参数: 
 ***    参数1: dst	目的字符串
 *** 返 回 值: 0 成功 -1	失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_substring(char *dst, char *src, size_t start, size_t end)
{
	size_t i = 0;

	i = start;
	
	if(start > pub_str_strlen(src))
		return -1;

	if(end > pub_str_strlen(src))
		end = pub_str_strlen(src);

	while(i < end)
	{
		dst[i - start] = src[i];
		i++;
	}

	dst[i - start] = '\0';

	return 0;

}

/******************************************************************************
 *** 函数名称: pub_str_charat
 *** 函数功能: 返回字符串中索引为index的字符
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: src	源字符串
 ***    参数2: index	索引
 *** 输出参数: 
 *** 返 回 值: 匹配字符
 *** 注意事项: 
 ******************************************************************************/
char pub_str_charat(char *src, size_t index)
{
	char *p;
	size_t i = 0;

	p = src;
	if(index > pub_str_strlen(src))
		return 0;

	while(i < index )
		i++;

	return p[i];
}

/******************************************************************************
 *** 函数名称: pub_str_indexof
 *** 函数功能: 找出str2字符串在str1字符串中第一次出现的位置
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: str1	源字符串
 ***    参数2: str2	匹配字符串
 *** 输出参数: 
 *** 返 回 值: >=0 成功  <0 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_indexof(char *str1, char *str2)
{
	char *p;
	int pos = 0;
	
	p = str1;
	p = pub_str_strstr(str1, str2);
	if(p == NULL)
	{
		return -1;
	}
	else
	{
		while(str1 != p)
		{
			str1++;
			pos++;
		}
	}
	
	return pos;
}

/******************************************************************************
 *** 函数名称: pub_str_lastindexof
 *** 函数功能: 找出str2字符串在str1字符串中最后一次出现的位置
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: str1	源字符串
 ***    参数2: str2	匹配字符串
 *** 输出参数: 
 *** 返 回 值: >=0 成功  <0 失败
 *** 注意事项: 
 ******************************************************************************/
int pub_str_lastindexof(char *str1, char *str2)
{
	char *p ;
	int pos = 0;
	int len = 0;

	p = str1;
	len = pub_str_strlen(str2);

	p = pub_str_strstr(str1, str2);
	if(p == NULL)
	{
		return -1;
	}
	
	while(p != NULL)
	{
		for(; str1 != p; str1++)
			pos++;
		p = p + len;
		p = pub_str_strstr(p, str2);
	}
	
	return pos;
}

/* * supported formats: 
*    %[0][width][x][X]O        off_t 
*    %[0][width]T              time_t 
*    %[0][width][u][x|X]z      ssize_t/size_t 
*    %[0][width][u][x|X]d      int/u_int 
*    %[0][width][u][x|X]l      long 
*    %[0][width|m][u][x|X]i    sw_int_t/sw_uint_t 
*    %[0][width][u][x|X]D      int32_t/uint32_t 
*    %[0][width][u][x|X]L      int64_t/uint64_t 
*    %[0][width][.width]f      double, max valid number fits to %18.15f 
*    %P                        sw_pid_t 
*    %M                        sw_msec_t 
*    %r                        rlim_t 
*    %p                        void * 
*    %V                        sw_str_t * 
*    %v                        sw_variable_value_t * 
*    %s                        null-terminated string 
*    %*s                       length and string 
*    %Z                        '\0' 
*    %N                        '\n' 
*    %c                        char 
*    %%                        % 
* 
*  reserved: 
*    %t                        ptrdiff_t 
*    %S                        null-terminated wchar string 
*    %C                        wchar 
*/
/******************************************************************************
 *** 函数名称: pub_str_sprintf
 *** 函数功能: 把各种类型的数据格式化输出到buf，最大的长度为65536
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: buf	缓冲区
 ***    参数2: fmt	指定格式串
 *** 输出参数: 
 *** 返 回 值: 缓冲区内容
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_sprintf(sw_char_t *buf, const char *fmt, ...)
{    
	sw_char_t   *p;    
	va_list   args;    
	va_start(args, fmt);    
	p = pub_str_vslprintf(buf, (void *) -1, fmt, args);    
	va_end(args);    
	return p;
}

/******************************************************************************
 *** 函数名称: pub_str_snprintf
 *** 函数功能: 把各种类型的数据格式化输出到指定长度的buf
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: buf	缓冲区
 ***    参数2: max	缓冲区长度
 ***    参数3: fmt	指定格式串
 *** 输出参数: 
 *** 返 回 值: 缓冲区内容
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_snprintf(sw_char_t *buf, size_t max, const char *fmt, ...)
{
	sw_char_t   *p;
	va_list   args;
	
	va_start(args, fmt);
	p = pub_str_vslprintf(buf, buf + max, fmt, args);
	va_end(args);
	
	return p;
}

/******************************************************************************
 *** 函数名称: pub_str_slprintf
 *** 函数功能: 把各种类型的数据格式化输出到指定长度的buf
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: buf	缓冲区
 ***    参数2: last	缓冲区界定指针
 ***    参数3: fmt	指定格式串
 *** 输出参数: 
 *** 返 回 值: 缓冲区内容
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_slprintf(sw_char_t *buf, sw_char_t *last, const char *fmt, ...)
{
	sw_char_t   *p;
	va_list   args;
	
	va_start(args, fmt);
	p = pub_str_vslprintf(buf, last, fmt, args);
	va_end(args);
	
	return p;
}

/******************************************************************************
 *** 函数名称: pub_str_vslprintf
 *** 函数功能: 把各种类型的数据格式化输出到指定的buf
 *** 函数作者: liuyong
 *** 生成日期: 2012-7-27 11:09:14
 *** 调用函数: 
 *** 	函数1: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 ***    参数1: buf	缓冲区
 ***    参数2: last	缓冲区界定指针
 ***    参数3: fmt	指定格式串
 *** 输出参数: 
 *** 返 回 值: 缓冲区内容
 *** 注意事项: 
 ******************************************************************************/
sw_char_t *pub_str_vslprintf(sw_char_t *buf, sw_char_t *last, const char *fmt, va_list args)
{
	sw_char_t                *p, zero;
	int                    d;
	double                 f, scale;
	size_t                 len, slen;
	int64_t                i64;
	uint64_t               ui64;
	int			ms;
	unsigned int           width, sign, hex, max_width, frac_width, n;
	sw_str_t             *v;
	sw_variable_value_t  *vv;
    
	while (*fmt && buf < last) 
	{
		/*
		 * "buf < last" means that we could copy at least one character:
		 * the plain character, "%%", "%c", and minus without the checking
		 */
	
		if (*fmt == '%') 
		{
			i64 = 0;
			ui64 = 0;
			
			zero = (sw_char_t) ((*++fmt == '0') ? '0' : ' ');
			width = 0;
			sign = 1;
			hex = 0;
			max_width = 0;
			frac_width = 0;
			slen = (size_t) -1;
		
			while (*fmt >= '0' && *fmt <= '9') 
			{
				width = width * 10 + *fmt++ - '0';
			}
		
			for ( ;; ) 
			{
				switch (*fmt) 
				{
				
					case 'u':
					    sign = 0;
					    fmt++;
					    continue;
				
					case 'm':
					    max_width = 1;
					    fmt++;
					    continue;
					
					case 'X':
					    hex = 2;
					    sign = 0;
					    fmt++;
					    continue;
				
					case 'x':
					    hex = 1;
					    sign = 0;
					    fmt++;
					    continue;
					
					case '.':
					    fmt++;
					
					    while (*fmt >= '0' && *fmt <= '9') 
					    {
					        frac_width = frac_width * 10 + *fmt++ - '0';
					    }
					
					    break;
					
					case '*':
					    slen = va_arg(args, size_t);
					    fmt++;
					    continue;
					
					default:
					    break;
					}
					
				break;
			}
		    
			switch (*fmt) 
			{
				case 'V':
					v = va_arg(args, sw_str_t *);
					
					len = pub_math_min(((size_t) (last - buf)), v->len);
					buf = (sw_char_t*)memcpy(buf, v->data, len);
					fmt++;
					
					continue;
			
				case 'v':
					vv = va_arg(args, sw_variable_value_t *);
					
					len = pub_math_min(((size_t) (last - buf)), vv->len);
					buf = (sw_char_t*)memcpy(buf, vv->data, len);
					fmt++;
					
					continue;
			
				case 's':
					p = va_arg(args, sw_char_t *);
					if (slen == (size_t) -1) 
					{
						while (*p && buf < last) 
						{
							*buf++ = *p++;
						}
					
					} 
					else 
					{
						len = pub_math_min(((size_t) (last - buf)), slen);
						buf = (sw_char_t*)memcpy(buf, p, len);
					}
					
					fmt++;
					continue;
			
				case 'O':
					i64 = (int64_t) va_arg(args, off_t);
					sign = 1;
					break;
			
				case 'P':
					i64 = (int64_t) va_arg(args, pid_t);
					sign = 1;
					break;
			
				case 'T':
					i64 = (int64_t) va_arg(args, time_t);
					sign = 1;
					break;
			
				case 'M':
					ms = (int) va_arg(args, int);
					if ((int) ms == -1) 
					{
						sign = 1;
						i64 = -1;
					} 
					else 
					{
						sign = 0;
						ui64 = (uint64_t) ms;
					}
					break;
			
				case 'z':
					if (sign) 
					{
					    i64 = (int64_t) va_arg(args, ssize_t);
					} 
					else 
					{
					    ui64 = (uint64_t) va_arg(args, size_t);
					}
					break;
			
				case 'i':
					if (sign) 
					{
					    i64 = (int64_t) va_arg(args, int);
					} 
					else 
					{
					    ui64 = (uint64_t) va_arg(args, int);
					}
					
					if (max_width) 
					{
					    width = SW_INT_T_LEN;
					}
					
					break;
			
				case 'd':
					if (sign) 
					{
					    i64 = (int64_t) va_arg(args, int);
					} 
					else 
					{
					    ui64 = (uint64_t) va_arg(args, u_int);
					}
					break;
			
				case 'l':
					if (sign) 
					{
					    i64 = (int64_t) va_arg(args, long);
					} 
					else 
					{
					    ui64 = (uint64_t) va_arg(args, u_long);
					}
					break;
			
				case 'D':
					if (sign) 
					{
					    i64 = (int64_t) va_arg(args, int32_t);
					} 
					else 
					{
					    ui64 = (uint64_t) va_arg(args, uint32_t);
					}
					break;
			
				case 'L':
					if (sign) 
					{
					    i64 = va_arg(args, int64_t);
					} 
					else 
					{
					    ui64 = va_arg(args, uint64_t);
					}
					break;
			
				case 'f':
					f = va_arg(args, double);
			
					if (f < 0) 
					{
					    *buf++ = '-';
					    f = -f;
					}
			
					ui64 = (int64_t) f;
					
					buf = pub_str_sprintf_num(buf, last, ui64, zero, 0, width);
			
					if (frac_width) 
					{
					
					    if (buf < last) 
					    {
					        *buf++ = '.';
					    }
					
					    scale = 1.0;
					
					    for (n = frac_width; n; n--) 
					    {
					        scale *= 10.0;
					    }
					
					    /*
					     * (int64_t) cast is required for msvc6:
					     * it can not convert uint64_t to double
					     */
					    ui64 = (uint64_t) ((f - (int64_t) ui64) * scale + 0.5);
					
					    buf = pub_str_sprintf_num(buf, last, ui64, '0', 0, frac_width);
					}
					
					fmt++;
					
					continue;
			
				case 'r':
					i64 = (int64_t) va_arg(args, rlim_t);
					sign = 1;
					break;
			
				case 'p':
					ui64 = (uintptr_t) va_arg(args, void *);
					hex = 2;
					sign = 0;
					zero = '0';
					width = SW_PTR_SIZE * 2;
					break;
			
				case 'c':
					d = va_arg(args, int);
					*buf++ = (sw_char_t) (d & 0xff);
					fmt++;
					
					continue;
			
				case 'Z':
					*buf++ = '\0';
					fmt++;
					
					continue;
			
				case 'N':
					*buf++ = LF;
					fmt++;
					
					continue;
			
				case '%':
					*buf++ = '%';
					fmt++;
					
					continue;
			
				default:
					*buf++ = *fmt++;
			
					continue;
			}
		
			if (sign) 
			{
				if (i64 < 0) 
				{
					*buf++ = '-';
					ui64 = (uint64_t) -i64;
				} 
				else 
				{
					ui64 = (uint64_t) i64;
				}
			}
		
		    	buf = pub_str_sprintf_num(buf, last, ui64, zero, hex, width);
		
		    	fmt++;
		} 
		else 
		{
			*buf++ = *fmt++;
		}
	}

    	return buf;
}

/* 检查是否有半个汉字 */
sw_int_t pub_str_chkhalf(char *buf, size_t len)
{
	register char *p, *q;
	register enum { DCODE, SCODE, DSCODE } estat;
	
	estat = SCODE;
	q = buf + len;
	
	for(p = buf; p != q; p++) 
	{
		switch(estat)
		{
			case DCODE:
				if(*p & 0x80) 
					estat = DSCODE;
				else 
					estat = SCODE;
				break;
			case SCODE:
				if(*p & 0x80) 
					estat = DSCODE;
				break;
			case DSCODE:
				if(*p & 0x80) 
				{
					estat = DCODE;
				}
				else 
				{                                                                                         
					return SW_ERROR;                                                                                 
				}                                                                                              
				break;                                                                                         
		}                                                                                                  
	}
	                                                                                                  
	return SW_OK;                                                                                              
}

int pub_str_chkchinese(char *str)
{
	struct
	{
		char en[2]; 
		char cn[3];
	} data[13]= { 	
				{ ":" , "：" } , 
				{ "'" , "  " } , 
				{ "\"", "  " } ,
				{ "\\", "  " } ,
				{ "%" , "％" } ,
				{ "{" , "｛" } , 
				{ "}" , "｝" } ,
				{ "[" , "［" } ,
				{ "]" , "］" } ,
				{ "(" , "（" } ,
				{ ")" , "）" } ,
				{ "<" , "《" } ,
				{ ">" , "》" } ,
		};
		
	int i, j, k, len;
	
	char *p = NULL;
 	char buf[256];
 	
	len = 0;

	if( pub_str_chkhalf(str,strlen(str)) )
	{
		return SW_ERROR;
	}

 	memset( buf, 0x00, sizeof( buf ) );

 	p = str ;
 	len = strlen(str);
	for( i = k = 0 ; i < len ; i++ )
	{
	    	for( j = 0 ; j < 13 ; j ++ )
	    	{
	       		if( *p == data[j].en[0] )
	       		{
	           		memcpy(buf + k,data[j].cn , 2 );
	           		k++;
					break;
	       		}
	       		else
	          		buf[k] = *p;
	    	}
	    	p ++; k ++ ;
	}

	if(k > len)
	{
		return SW_ERROR;
	}
 	memcpy( str ,  buf , k );
	str[k] = '\0';

	return SW_OK;	
}



/**********************************
从instr中截取第一个delimiter之前的内容放到outstr中，返回第一个delimiter之后的位置
**************************************/
char *pub_str_msstrtok(char *instr, char *outstr, char *delimiter)
{
	sw_char_t	*tmpstr = NULL;

	if (memcmp(instr, delimiter, strlen(instr)) == 0)
	{
		return(NULL);
  	}

  	if (instr == NULL || strlen(instr) == 0)
  	{
  		return NULL;
  	}

  	tmpstr = strstr(instr, delimiter);

  	if( tmpstr != NULL)
  	{
  		memcpy(outstr, instr, strlen(instr) - strlen(tmpstr));

  		return (strstr(instr, delimiter) + strlen(delimiter));
  	}
  	else
  	{
  		memcpy(outstr, instr, strlen(instr));
  		return NULL;
  	}
}

/****能处理\xNN转义的strcpy版本****/
int pub_str_strcpy_ext(char *s1,char *s2)
{
	int i,j;
	i=j=0;
	while(s2[i]!='\0'){
		if(s2[i]=='\\' && s2[i+1]=='x'){
			char sTmp[2];
			pub_code_asctobcd(s2+i+2,sTmp,2);
			s1[j]=sTmp[0];
			i+=4;
			j++;
		}else{
			s1[j]=s2[i];
			i++;
			j++;
		}
	}
	s1[j]='\0';
	return(j);
}
/* check include */
sw_int_t pub_str_checkincl(sw_char_t *include,sw_char_t *str)
{
	sw_int32_t i,n;
	sw_char_t check_value[32];

	i = 0;
	n = 0;
	while(include[i] != 0)
	 {
		 if (include[i] != ' ')
		 {
			 check_value[n] = include[i];
			 n++;
			 i++;
		 }
		 else
		 {
			 check_value[n] = '\0';
			 n = 0;
			 i++;
 
			 if (strcmp(str,check_value) == 0)
			 {
				 return SW_OK;
			 }
			 else
			 {
				 memset(check_value,0x00 ,sizeof(check_value));
			 }
		 }
	 }

	return SW_ERR;
}

/******************************************************************************
 **函数名称: str_isdigit
 **功    能: 判断字符串是否为数字
 **输入参数: 
 **     str: 要被判断的字符串
 **输出参数: NONE
 **返    回: true：是  false：不是
 **实现描述: 
 **     依次判断各字符是否为数字
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
bool str_isdigit(const char *str)
{
    const char *p = str;
    sw_int_t idx = 0, len = strlen(str);

    if(0 == len)
    {
        return false;
    }
	
    for(idx=0; idx<len; idx++)
    {
        if(!isdigit(*p))
        {
            return false;
        }
    }
    
    return true;
}

int pub_str_strrncmp(char *s1, char *s2, size_t n)
{
	char    *p1 = NULL;
	char    *p2 = NULL;

	if (s1 == NULL && s2 == NULL)
	{
		return 0;
	}

	if (s1 == NULL || s2 == NULL)
	{
		return -1;
	}

	p1 = s1 + strlen(s1) - 1;
	p2 = s2 + strlen(s2) - 1;
	while (p1 >= s1 && p2 >= s2 && n > 0)
	{
		if (*p1 != *p2)
		{
			break;
		}
		p1--;
		p2--;
		n--;
	}

	if (n == 0)
	{
		return 0;
	}

	return -1;
}

int pub_str_include(char *var, char *str)
{
	char    *ptr = NULL;
	char    *ptmp = NULL;
	char    value[512];
	const char      sep = ' ';

	memset(value, 0x0, sizeof(value));

	ptr = str;
	ptmp = value;
	while (*ptr != '\0')
	{
		if (*ptr == sep)
		{
			*ptmp = '\0';
			ptr++;
			if (strcmp(value, var) == 0)
			{
				return 0;
			}
			memset(value, 0x0, sizeof(value));
			ptmp = value;
			continue;
		}

		*ptmp++ = *ptr++;
	}
	if (strcmp(value, var) == 0)
	{
		return 0;
	}

	return -1;
}

