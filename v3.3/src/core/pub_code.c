/*********************************************************************
 *** version : v2.0
 *** author  : baidan
 *** create  : 2012-7-27 17:10:43
 *** module  : base 
 *** name    : pub_code.c 
 *** function: code convert
 *** lists   :
 ***		pub_code_asctobcd	   ASCII to BCD
 ***		pub_code_bcdtoasc     	   BCD to ASCII
 ***		pub_code_base64_enc        base64 enc 
 ***		pub_code_base64_dec        base64 dec 
 ***		pub_code_convert           code convert 
 ***		pub_code_u2g               UNICODE to GBK
 ***		pub_code_g2u               GBK to UNICODE
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "pub_code.h"
#include <arpa/inet.h>
#include "pub_log.h"

static unsigned char base64[]=
{
	'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N',
	'O','P','Q','R','S','T',
	'U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g',
	'h','i','j','k','l','m','n',
	'o','p','q','r','s','t',
	'u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',
	'+','/'
};

/******************************************************************************
 *** name      : pub_code_base64_enc
 *** function  : base64 enc
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists: 
 ***  function1: none 
 *** inputs    : 
 *** 	  arg1 : src_code	source message	
 *** 	  arg2 : src_len	source message's length	
 *** outputs   : 
 *** 	  arg1 : dst_code	destination message 
 *** return    : 0:success  -1:fail
 *** notice    : 
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pub_code_base64_enc(unsigned char *src_code, unsigned char *dst_code, 
	sw_int_t src_len)
{
	if (src_code == NULL || src_len <= 0)
	{
		return SW_ERROR;
	}
	
	sw_int_t	i, j;
	sw_int_t	sum, dum;
	sw_int_t	src_pos, tgt_pos;
	sw_int_t	tgt_len, tgt_eqlen;
	
	unsigned char	buf[3];
	unsigned char	tmp[4];
	
	i = j = 0;
	sum = dum = 0;
	src_pos = tgt_pos = 0;
	tgt_len = tgt_eqlen =0;
	
	memset(buf, 0x00, sizeof(buf));
	memset(tmp, 0x00, sizeof(tmp));
	
	if (src_len % 3 == 0)
	{
		tgt_len = src_len / 3 * 4;
		tgt_eqlen = 0;
	} 
	else
	{
		tgt_len = (src_len / 3 + 1) * 4;
		tgt_eqlen = 3 - (src_len % 3);
	}
	
	while (src_pos < src_len)
	{
		if (src_pos + 3 > src_len)
		{
			/*less than 3 bytes*/
			memcpy((char *)buf, (char *)(src_code + src_pos), src_len - src_pos);
			memset((char *)(buf + src_len - src_pos), '\0', 3 - src_len + src_pos);
		}
		else
		{
			/*large than 3 bytes*/
			memcpy((char *)buf, (char *)(src_code + src_pos), 3);
		}
		
		sum = 0;
		for (i = 0; i < 3; i++)
		{
			sum *= 256;
			sum += buf[i];
		}
		
		dum = 64 * 64 * 64;
		for (i = 0; i < 4; i++)
		{
			tmp[i] = base64[sum / dum];
			sum = sum % dum;
			dum /= 64;
		}

		memcpy((char *)(dst_code + tgt_pos), (char *)tmp, 4);
		tgt_pos += 4;
		src_pos += 3;
	}
	
	if (tgt_eqlen != 0)
	{
		memset((char *)(dst_code + tgt_len - tgt_eqlen), '=', tgt_eqlen);
	}

	dst_code[tgt_len] = '\0';
	
	return tgt_len;
}

/******************************************************************************
 *** name      : pub_code_base64_dec
 *** function  : base64 dec 
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : src_code	source message
 *** outputs   :
 ***      arg1 : dst_code	destination message
 ***      arg2 : dst_len	destination message's length
 *** return    : 0:success  	-1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
******************************************************************************/
sw_int_t pub_code_base64_dec(unsigned char *src_code, unsigned char *dst_code, 
	sw_int_t *dst_len)
{
	if (src_code == NULL || strlen((sw_char_t*)src_code) == 0)
	{
		return SW_ERROR;
	}
	
	sw_int_t	i, j;
	sw_int_t	sum, dum;
	sw_int_t	src_pos, tgt_pos, src_len;

	unsigned char	buf[3];
	unsigned char	tmp[4];

	i = j = 0;
	sum = dum = 0;
	src_pos = tgt_pos = src_len =0;

	memset(buf, 0x00, sizeof(buf));
	memset(tmp,0x00, sizeof(tmp));
	
	src_len = strlen((char *)src_code);

	if (src_len % 4 != 0)
	{
		return SW_ERROR;
	}
		
	if (src_code[src_len - 2] == '=')
	{
		*dst_len = src_len / 4 * 3 - 2;
	} 
	else if(src_code[src_len - 1] == '=')
	{
		*dst_len = src_len / 4 * 3 - 1;
	} 
	else
	{
		*dst_len = src_len / 4 * 3;
	}
	
	while (src_pos < src_len)
	{
		memcpy(tmp, (src_code + src_pos), 4);
		
		sum = 0;
		for (i = 0; i < 4; i++)
		{
			int ch;
			ch = src_code[src_pos + i];
			if (ch >= 'A' && ch <= 'Z')
			{
				ch = ch - 'A';
			} 
			else if(ch >='a' && ch<='z')
			{
				ch = ch - 'a' + 26;
			} 
			else if(ch >= '0' && ch <= '9')
			{
				ch = ch - '0' + 52;
			}
			else if(ch == '+')
			{
				ch = 62;
			} 
			else if(ch == '/')
			{
				ch = 63;
			} 
			else if(ch == '=')
			{
				ch = 0;
			} 
			else
			{
				return SW_ERROR;
			}
			sum *= 64;
			sum += ch;
		}
		
		dum = 256 * 256;
		
		for (i = 0; i < 3; i++)
		{
			buf[i] = sum / dum;
			sum = sum % dum;
			dum /= 256;
		}
		
		if (tgt_pos + 3 > *dst_len)
		{
			memcpy((char *)(dst_code + tgt_pos), (char *)buf, *dst_len - tgt_pos);
		} 
		else
		{
			memcpy((char *)(dst_code + tgt_pos), (char *)buf, 3);
		}

		tgt_pos += 3;
		src_pos += 4;
	}

	return SW_OK;
}

/******************************************************************************
 *** name      : pub_code_convert
 *** function  : code convert 
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : from		source charset 
 ***      arg2 : to		destination charset 
 ***      arg3 : src		source message 
 ***      arg4 : src_len	source message's length 
 *** outputs   :
 ***      arg1 : dst		destination message
 ***      arg2 : len		nouse space of output buffer 	
 *** return    : 0:success      -1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pub_code_convert(char *from, char *to, char *src, size_t *src_len, 
	char *dst, size_t *len)
{
	if (from == NULL || to == NULL || src == NULL || *src_len <= 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	iconv_t	cd;
	char 	**input;
	char 	**output;
	
	input = &src;
	output = &dst;

	cd = iconv_open(to, from);
	if (cd == 0)
	{
		pub_log_error("[%s][%d] iconv_open error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	if (iconv(cd, input, src_len, output, len) == (size_t)-1)
	{
		pub_log_error("[%s][%d] iconv error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return SW_ERROR;
	}

	iconv_close(cd);
	
	return SW_OK;
}

/******************************************************************************
 *** name      : pub_code_u2g 
 *** function  : unicode to gbk 
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : src		source message 
 ***      arg2 : src_len	source message's length	
 *** outputs   :
 ***      arg1 : dst		destination message
 ***      arg2 : dst_len	nouse space of output buffer's size	
 *** return    : 0:success      -1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pub_code_u2g(char *src, size_t *src_len, char *dst, size_t *dst_len)
{
	if (src == NULL || *src_len <= 0 || *dst_len <= 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return pub_code_convert("UTF-8", "GBK", src, src_len, dst, dst_len);
}

/******************************************************************************
 *** name      : pub_code_g2u
 *** function  : gbk to unicode 
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : src		source message
 ***      arg2 : src_len        source message's length
 *** outputs   :
 ***      arg1 : dst            destination message
 ***      arg2 : dst_len        nouse space of output buffer's size
 *** return    : 0:success      -1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pub_code_g2u(char *src, size_t *src_len, char *dst, size_t *dst_len)
{
	if (src == NULL || *src_len <= 0 || *dst_len <= 0)
	{
		return SW_ERROR;
	}
	
	return pub_code_convert("GBK", "UTF-8", src, src_len, dst, dst_len);
}

/******************************************************************************
 *** name      : pub_code_asctobcd
 *** function  : ascii to bcd 
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : src            source message
 ***      arg2 : src_len        source message's length
 *** outputs   :
 ***      arg1 : dst            destination message
 *** return    : 0:success      -1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
 ******************************************************************************/
sw_int_t pub_code_asctobcd(char *src, char *dst, sw_int_t src_len)
{
	if (src == NULL || src_len <= 0)
	{
		return SW_ERROR;
	}
	
	sw_int_t	i;
	unsigned char 	ch1;
	unsigned char 	ch2;
	
	for(i = 0; i < src_len / 2; i++)
	{
		ch1 = (unsigned char)src[i * 2];
		ch2 = (unsigned char)src[i * 2 + 1];
		
		if (ch1 >= 'a' && ch1 <= 'f')
		{
			ch1 = ch1 - 'a' + 0xa;
		}	
		else if (ch1 >= 'A' && ch1 <= 'F')
		{
			ch1 = ch1 - 'A' + 0xa;
		}	
		else if (ch1 >= '0' && ch1 <='9')
		{
			ch1 = ch1 - '0';
		}	
		
		if (ch2 >= 'a' && ch2 <= 'f')
		{
			ch2 = ch2 - 'a' + 0xa;
		}	
		else if(ch2 >= 'A' && ch2 <= 'F')
		{
			ch2 = ch2 - 'A' + 0xa;
		}	
		else if (ch2 >= '0' && ch2 <='9')
		{
			ch2 = ch2 - '0';
		}	
		
		dst[i] = (ch1 << 4) | ch2;
	}
	
	return SW_OK;
}

sw_int_t pub_code_asctobcd_right(char *src, char *dst, sw_int_t src_len)
{
	unsigned char ch1;

	if ((src_len % 2) != 0)
	{
		ch1 = (unsigned char)*src;
		if (ch1 >= 'a' && ch1 <= 'f')
		{
			ch1 = ch1 - 'a' + 0xa;
		}	
		else if (ch1 >= 'A' && ch1 <= 'F')
		{
			ch1 = ch1 - 'A' + 0xa;
		}	
		else if (ch1 >= '0' && ch1 <= '9')
		{
			ch1 = ch1 - '0';
		}

		*dst |= ch1;
		src++;
		dst++;
		src_len--;
	}

	pub_code_asctobcd(src, dst, src_len);
	return 0;
}


sw_int_t pub_code_asctobcd_left(char *src, char *dst, sw_int_t src_len)
{
	if ((src_len % 2) != 0)  
	{  
		dst[src_len - 1] = '0';  
		src_len++; 
	}

	pub_code_asctobcd(src, dst, src_len);
	return 0;
}

/******************************************************************************
 *** name      : pub_code_bcdtoasc
 *** function  : bcd to ascii
 *** author    : baidan
 *** create    : 2012-7-27 17:11:35
 *** call lists:
 ***  function1: none
 *** inputs    :
 ***      arg1 : src		source message
 ***      arg2 : src_len	source message's length
 *** outputs   :
 ***      arg1 : dst		destination message
 *** return    : 0:success      -1:fail
 *** notice    :
 *** modified  :
 ***    author :
 ***    date   :
 ***    content:
******************************************************************************/
sw_int_t pub_code_bcdtoasc(char *src, char *dst, sw_int_t src_len)
{
	if (src == NULL || src_len <= 0)
	{
		return SW_ERROR;
	}
	
	sw_int_t	i;
	unsigned char 	ch;
	
	for (i = 0; i < src_len; i++)
	{
		ch = (unsigned char)src[i];
		ch = ch >> 4;
		
		if (ch >= 10)
		{
			dst[2*i] = ch - 10 + 'A';
		}
		else
		{
			dst[2 * i] = ch + '0';
		}
		
		ch = (unsigned char)src[i];
		ch = ch & 0x0f;
		if (ch >= 10)
		{
			dst[2 * i + 1]= ch - 10 + 'A';
		}
		else
		{
			dst[2 * i + 1] = ch + '0';
		}
	}
	
	return SW_OK;
}

sw_int_t pub_code_numtohost(int num,char *buf,int len)
{
	sw_int32_t	ret_int = 0;
	sw_int16_t	ret_short = 0;
	
	if (sizeof(int) != 4  || sizeof(short) != 2)
	{
		printf("This system can not run on the OS whose bits-num less than 32.\n");
		exit(1);
	}
	
	if (len == 2)
	{
		ret_short=(short)num;
		memcpy(buf,(char *)&ret_short,len);
	}
	else if (len == 4)
	{
		ret_int=(int)num;
		memcpy(buf,(char *)&ret_int,len);
	}
	else
	{
		printf("Data's length[%d] must be 2 or 4.\n\n", len);
		return SW_ERROR;
	}
	
	return SW_OK;
}


sw_int_t pub_code_numtonet(int num, char *buf, int len)
{
	sw_int32_t	ret_int = 0;
	sw_int16_t	ret_short = 0;
	
	if (sizeof(int) != 4  || sizeof(short) != 2)
	{
		printf("This system can not run on the OS whose bits-num less than 32.\n");
		exit(1);
	}
	
	if (len == 2)
	{
		ret_short = (sw_int16_t)num;
		ret_short = htons(ret_short);
		memcpy(buf, (sw_char_t *)&ret_short, len);
	}
	else if(len == 4)
	{
		ret_int = (int)num;
		ret_int = htonl(ret_int);
		memcpy(buf, (char *)&ret_int, len);
	}
	else
	{
		printf("Data's length[%d] must be 2 or 4.\n",len);
		return SW_ERROR;
	}
	
	return SW_OK;
}


sw_int_t pub_code_nettonum(char *buf,int len)
{
	sw_int32_t	ret_int = 0;
	sw_int16_t	ret_short = 0;
	
	if (sizeof(int) != 4  || sizeof(short) != 2)
	{
		printf("This system can not run on the OS whose bits-num less than 32.\n");
		exit(1);
	}
	
	if (len == 2)
	{
		memcpy((char *)&ret_short, buf,len);
		ret_short = ntohs(ret_short);
		return ((sw_int32_t)ret_short);
	}
	else if(len == 4)
	{
		memcpy((char *)&ret_int,buf,len);
		ret_int = ntohl(ret_int);
		return ((sw_int32_t)ret_int);
	}
	else
	{
		printf("Data's length[%d] must be 2 or 4.\n\n", len);
		return SW_ERROR;
	}
	
	return SW_OK;
}

/****把字符串按主机顺序转换成数字****/
sw_int_t pub_code_hosttonum(char *buf, int len)
{
	sw_int32_t	ret_int = 0;
	sw_int16_t	ret_short = 0;
	
	if (sizeof(int) != 4  || sizeof(short) != 2)
	{
		printf("This system can not run on the OS whose bits-num less than 32.\n");
		exit(1);
	}
	
	if (len == 2)
	{
		memcpy((char *)&ret_short,buf,len);
		return((int)ret_short);
	}
	else if (len == 4)
	{
		memcpy((char *)&ret_int,buf,len);
		return((int)ret_int);
	}
	else
	{
		printf("Data's length[%d] must be 2 or 4.\n\n", len);
		return SW_ERROR;
	}
	
	return SW_OK;
}

/*************************************************************************
* Constants used for LZ77 coding
*************************************************************************/

/* Maximum offset (can be any size < 2^31). Lower values give faster
   compression, while higher values gives better compression. The default
   value of 100000 is quite high. Experiment to see what works best for
   you. */
#define LZ_MAX_OFFSET 100000



/*************************************************************************
*                           INTERNAL FUNCTIONS                           *
*************************************************************************/


/*************************************************************************
* _LZ_StringCompare() - Return maximum length string match.
*************************************************************************/

static unsigned int _LZ_StringCompare( unsigned char * str1,
  unsigned char * str2, unsigned int minlen, unsigned int maxlen )
{
    unsigned int len;

    for( len = minlen; (len < maxlen) && (str1[len] == str2[len]); ++ len );

    return len;
}


/*************************************************************************
* _LZ_WriteVarSize() - Write unsigned integer with variable number of
* bytes depending on value.
*************************************************************************/

static int _LZ_WriteVarSize( unsigned int x, unsigned char * buf )
{
    unsigned int y;
    int num_bytes, i, b;

    /* Determine number of bytes needed to store the number x */
    y = x >> 3;
    for( num_bytes = 5; num_bytes >= 2; -- num_bytes )
    {
        if( y & 0xfe000000 ) break;
        y <<= 7;
    }

    /* Write all bytes, seven bits in each, with 8:th bit set for all */
    /* but the last byte. */
    for( i = num_bytes-1; i >= 0; -- i )
    {
        b = (x >> (i*7)) & 0x0000007f;
        if( i > 0 )
        {
            b |= 0x00000080;
        }
        *buf ++ = (unsigned char) b;
    }

    /* Return number of bytes written */
    return num_bytes;
}


/*************************************************************************
* _LZ_ReadVarSize() - Read unsigned integer with variable number of
* bytes depending on value.
*************************************************************************/

static int _LZ_ReadVarSize( unsigned int * x, unsigned char * buf )
{
    unsigned int y, b, num_bytes;

    /* Read complete value (stop when byte contains zero in 8:th bit) */
    y = 0;
    num_bytes = 0;
    do
    {
        b = (unsigned int) (*buf ++);
        y = (y << 7) | (b & 0x0000007f);
        ++ num_bytes;
    }
    while( b & 0x00000080 );

    /* Store value in x */
    *x = y;

    /* Return number of bytes read */
    return num_bytes;
}



/*************************************************************************
*                            PUBLIC FUNCTIONS                            *
*************************************************************************/


/*************************************************************************
* LZ_Compress() - Compress a block of data using an LZ77 coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 0.4% larger
*           than the input buffer, plus one byte.
*  insize - Number of input bytes.
* The function returns the size of the compressed data.
*************************************************************************/

sw_int_t pub_code_Compress( unsigned char *in, unsigned char *out,
    unsigned int insize )
{
    unsigned char marker, symbol;
    unsigned int  inpos, outpos, bytesleft, i;
    unsigned int  maxoffset, offset, bestoffset;
    unsigned int  maxlength, length, bestlength;
    unsigned int  histogram[ 256 ];
    unsigned char *ptr1, *ptr2;

    /* Do we have anything to compress? */
    if( insize < 1 )
    {
        return 0;
    }

    /* Create histogram */
    for( i = 0; i < 256; ++ i )
    {
        histogram[ i ] = 0;
    }
    for( i = 0; i < insize; ++ i )
    {
        ++ histogram[ in[ i ] ];
    }

    /* Find the least common byte, and use it as the marker symbol */
    marker = 0;
    for( i = 1; i < 256; ++ i )
    {
        if( histogram[ i ] < histogram[ marker ] )
        {
            marker = i;
        }
    }

    /* Remember the marker symbol for the decoder */
    out[ 0 ] = marker;

    /* Start of compression */
    inpos = 0;
    outpos = 1;

    /* Main compression loop */
    bytesleft = insize;
    do
    {
        /* Determine most distant position */
        if( inpos > LZ_MAX_OFFSET ) maxoffset = LZ_MAX_OFFSET;
        else                        maxoffset = inpos;

        /* Get pointer to current position */
        ptr1 = &in[ inpos ];

        /* Search history window for maximum length string match */
        bestlength = 3;
        bestoffset = 0;
        for( offset = 3; offset <= maxoffset; ++ offset )
        {
            /* Get pointer to candidate string */
            ptr2 = &ptr1[ -(int)offset ];

            /* Quickly determine if this is a candidate (for speed) */
            if( (ptr1[ 0 ] == ptr2[ 0 ]) &&
                (ptr1[ bestlength ] == ptr2[ bestlength ]) )
            {
                /* Determine maximum length for this offset */
                maxlength = (bytesleft < offset ? bytesleft : offset);

                /* Count maximum length match at this offset */
                length = _LZ_StringCompare( ptr1, ptr2, 0, maxlength );

                /* Better match than any previous match? */
                if( length > bestlength )
                {
                    bestlength = length;
                    bestoffset = offset;
                }
            }
        }

        /* Was there a good enough match? */
        if( (bestlength >= 8) ||
            ((bestlength == 4) && (bestoffset <= 0x0000007f)) ||
            ((bestlength == 5) && (bestoffset <= 0x00003fff)) ||
            ((bestlength == 6) && (bestoffset <= 0x001fffff)) ||
            ((bestlength == 7) && (bestoffset <= 0x0fffffff)) )
        {
            out[ outpos ++ ] = (unsigned char) marker;
            outpos += _LZ_WriteVarSize( bestlength, &out[ outpos ] );
            outpos += _LZ_WriteVarSize( bestoffset, &out[ outpos ] );
            inpos += bestlength;
            bytesleft -= bestlength;
        }
        else
        {
            /* Output single byte (or two bytes if marker byte) */
            symbol = in[ inpos ++ ];
            out[ outpos ++ ] = symbol;
            if( symbol == marker )
            {
                out[ outpos ++ ] = 0;
            }
            -- bytesleft;
        }
    }
    while( bytesleft > 3 );

    /* Dump remaining bytes, if any */
    while( inpos < insize )
    {
        if( in[ inpos ] == marker )
        {
            out[ outpos ++ ] = marker;
            out[ outpos ++ ] = 0;
        }
        else
        {
            out[ outpos ++ ] = in[ inpos ];
        }
        ++ inpos;
    }

    return outpos;
}


/*************************************************************************
* LZ_CompressFast() - Compress a block of data using an LZ77 coder.
*  in     - Input (uncompressed) buffer.
*  out    - Output (compressed) buffer. This buffer must be 0.4% larger
*           than the input buffer, plus one byte.
*  insize - Number of input bytes.
*  work   - Pointer to a temporary buffer (internal working buffer), which
*           must be able to hold (insize+65536) unsigned integers.
* The function returns the size of the compressed data.
*************************************************************************/

sw_int_t pub_code_CompressFast( unsigned char *in, unsigned char *out,
    unsigned int insize, unsigned int *work )
{
    unsigned char marker, symbol;
    unsigned int  inpos, outpos, bytesleft, i, index, symbols;
    unsigned int  offset, bestoffset;
    unsigned int  maxlength, length, bestlength;
    unsigned int  histogram[ 256 ], *lastindex, *jumptable;
    unsigned char *ptr1, *ptr2;

    /* Do we have anything to compress? */
    if( insize < 1 )
    {
        return 0;
    }

    /* Assign arrays to the working area */
    lastindex = work;
    jumptable = &work[ 65536 ];

    /* Build a "jump table". Here is how the jump table works:
       jumptable[i] points to the nearest previous occurrence of the same
       symbol pair as in[i]:in[i+1], so in[i] == in[jumptable[i]] and
       in[i+1] == in[jumptable[i]+1], and so on... Following the jump table
       gives a dramatic boost for the string search'n'match loop compared
       to doing a brute force search. The jump table is built in O(n) time,
       so it is a cheap operation in terms of time, but it is expensice in
       terms of memory consumption. */
    for( i = 0; i < 65536; ++ i )
    {
        lastindex[ i ] = 0xffffffff;
    }
    for( i = 0; i < insize-1; ++ i )
    {
        symbols = (((unsigned int)in[i]) << 8) | ((unsigned int)in[i+1]);
        index = lastindex[ symbols ];
        lastindex[ symbols ] = i;
        jumptable[ i ] = index;
    }
    jumptable[ insize-1 ] = 0xffffffff;

    /* Create histogram */
    for( i = 0; i < 256; ++ i )
    {
        histogram[ i ] = 0;
    }
    for( i = 0; i < insize; ++ i )
    {
        ++ histogram[ in[ i ] ];
    }

    /* Find the least common byte, and use it as the marker symbol */
    marker = 0;
    for( i = 1; i < 256; ++ i )
    {
        if( histogram[ i ] < histogram[ marker ] )
        {
            marker = i;
        }
    }

    /* Remember the marker symbol for the decoder */
    out[ 0 ] = marker;

    /* Start of compression */
    inpos = 0;
    outpos = 1;

    /* Main compression loop */
    bytesleft = insize;
    do
    {
        /* Get pointer to current position */
        ptr1 = &in[ inpos ];

        /* Search history window for maximum length string match */
        bestlength = 3;
        bestoffset = 0;
        index = jumptable[ inpos ];
        while( (index != 0xffffffff) && ((inpos - index) < LZ_MAX_OFFSET) )
        {
            /* Get pointer to candidate string */
            ptr2 = &in[ index ];

            /* Quickly determine if this is a candidate (for speed) */
            if( ptr2[ bestlength ] == ptr1[ bestlength ] )
            {
                /* Determine maximum length for this offset */
                offset = inpos - index;
                maxlength = (bytesleft < offset ? bytesleft : offset);

                /* Count maximum length match at this offset */
                length = _LZ_StringCompare( ptr1, ptr2, 2, maxlength );

                /* Better match than any previous match? */
                if( length > bestlength )
                {
                    bestlength = length;
                    bestoffset = offset;
                }
            }

            /* Get next possible index from jump table */
            index = jumptable[ index ];
        }

        /* Was there a good enough match? */
        if( (bestlength >= 8) ||
            ((bestlength == 4) && (bestoffset <= 0x0000007f)) ||
            ((bestlength == 5) && (bestoffset <= 0x00003fff)) ||
            ((bestlength == 6) && (bestoffset <= 0x001fffff)) ||
            ((bestlength == 7) && (bestoffset <= 0x0fffffff)) )
        {
            out[ outpos ++ ] = (unsigned char) marker;
            outpos += _LZ_WriteVarSize( bestlength, &out[ outpos ] );
            outpos += _LZ_WriteVarSize( bestoffset, &out[ outpos ] );
            inpos += bestlength;
            bytesleft -= bestlength;
        }
        else
        {
            /* Output single byte (or two bytes if marker byte) */
            symbol = in[ inpos ++ ];
            out[ outpos ++ ] = symbol;
            if( symbol == marker )
            {
                out[ outpos ++ ] = 0;
            }
            -- bytesleft;
        }
    }
    while( bytesleft > 3 );

    /* Dump remaining bytes, if any */
    while( inpos < insize )
    {
        if( in[ inpos ] == marker )
        {
            out[ outpos ++ ] = marker;
            out[ outpos ++ ] = 0;
        }
        else
        {
            out[ outpos ++ ] = in[ inpos ];
        }
        ++ inpos;
    }

    return outpos;
}


/*************************************************************************
* LZ_Uncompress() - Uncompress a block of data using an LZ77 decoder.
*  in      - Input (compressed) buffer.
*  out     - Output (uncompressed) buffer. This buffer must be large
*            enough to hold the uncompressed data.
*  insize  - Number of input bytes.
*************************************************************************/

sw_int_t pub_code_Uncompress( unsigned char *in, unsigned char *out,
    unsigned int insize ,unsigned int *outsize)
{
    unsigned char marker, symbol;
    unsigned int  i, inpos, outpos, length, offset;

    /* Do we have anything to uncompress? */
    if( insize < 1 )
    {
        return 0;
    }

    /* Get marker symbol from input stream */
    marker = in[ 0 ];
    inpos = 1;

    /* Main decompression loop */
    outpos = 0;
    do
    {
        symbol = in[ inpos ++ ];
        if( symbol == marker )
        {
            /* We had a marker byte */
            if( in[ inpos ] == 0 )
            {
                /* It was a single occurrence of the marker byte */
                out[ outpos ++ ] = marker;
                ++ inpos;
            }
            else
            {
                /* Extract true length and offset */
                inpos += _LZ_ReadVarSize( &length, &in[ inpos ] );
                inpos += _LZ_ReadVarSize( &offset, &in[ inpos ] );

                /* Copy corresponding data from history window */
                for( i = 0; i < length; ++ i )
                {
                    out[ outpos ] = out[ outpos - offset ];
                    ++ outpos;
                }
            }
        }
        else
        {
            /* No marker, plain copy */
            out[ outpos ++ ] = symbol;
        }
    }
    while( inpos < insize );
	*outsize = outpos;

	return 0;
}

int pub_code_int2hex(u_char *ptr, int value, int len)
{
	int	i = 0;
	int	t = 0;
	int	index = 0;
	int	size = len * 2;
	char	ch = 0;
	char	buf[32];
	u_char	val[3];

	memset(val, 0x0, sizeof(val));
	memset(buf, 0x0, sizeof(buf));
	sprintf(buf, "%0*x", size, value);
	for (i = 0; i < size; i++)
	{
		ch = buf[i];
		if (ch >= '0' && ch <= '9')
		{
			t = ch - '0';
		}
		else if (ch >= 'a' && ch <= 'f')
		{
			t = ch - 'a' + 10;
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			t = ch - 'A' + 10;
		}

		index = i / 2;
		val[index] <<= 4;
		val[index] |= t;
	}
	memcpy(ptr, val, len);

	return 0;
}

int pub_code_hex2int(u_char *ptr, int len)
{
	int	i = 0;
	int	val = 0;
	int	index = 0;

	for (i = len - 1; i >= 0; i--)
	{
		while (ptr[i])
		{
			if (index == 0)
			{
				val += (ptr[i] & 0x0F);
			}
			else
			{
				val += (ptr[i] & 0x0F) * (16 << (4 * (index - 1)));
			}
			ptr[i] >>= 4;
			index++;
		}
	}

	return val;
}

