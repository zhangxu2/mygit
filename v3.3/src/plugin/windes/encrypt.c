#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openssl/des.h"
#include "openssl/evp.h"
#include "lsn_pub.h"
#include "pub_type.h"

#define HEAD_LEN	8

int des_enc(char *buf, int *length)
{
	int	ret = 0;
	int	slen = 0;
	int	nlen = 0;
	int	ilen = 0;
	int	olen = 0;
	char	*ptr = NULL;
	DES_cblock	ivec = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};
	DES_cblock	keyEncrypt = {'D', 'H', 'C', 'C', 'S', 'o', 'f', 't'};
	EVP_ENCODE_CTX	ectx;
	DES_key_schedule	keySchedule;

	memset(&ectx, 0x0, sizeof(ectx));
	memset(&keySchedule, 0x0, sizeof(keySchedule));

	if (NULL == buf || NULL == length)
	{
		pub_log_info("[%s][%d] param is error", __FILE__, __LINE__);
		return -1;
	}

	ilen = *length;
	ptr = (char*)calloc(sizeof(char), 2*ilen);
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, 2*ilen, errno, strerror(errno));
		return -1;
	}
	nlen = (ilen % 8 ? (ilen / 8 + 1) * 8 : ilen);
	pub_log_info("[%s][%d] nlen=[%d] ilen=[%d]", __FILE__, __LINE__, nlen, ilen);
	memset(buf+ilen, nlen-ilen, nlen-ilen);
	DES_set_key_unchecked(&keyEncrypt, &keySchedule);
	DES_ncbc_encrypt(buf, ptr, nlen, &keySchedule, &ivec, DES_ENCRYPT);

	EVP_EncodeInit(&ectx);
	EVP_EncodeUpdate(&ectx, buf, &olen, ptr, nlen); 
	slen += olen;
	EVP_EncodeFinal(&ectx, buf + slen, &olen); 
	slen += olen;
	buf[slen] = '\0';
	*length = slen;
	free(ptr);

	return 0;
}

int des_dec(char *buf, int *length)
{
	int	ret = 0;
	int	slen = 0;
	int	nlen = 0;
	int	ilen = 0;
	int	olen = 0;
	char	*ptr = NULL;
	DES_cblock	ivec = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};
	DES_cblock	keyEncrypt = {'D', 'H', 'C', 'C', 'S', 'o', 'f', 't'};
	EVP_ENCODE_CTX	dctx;
	DES_key_schedule	keySchedule;

	memset(&dctx, 0x0, sizeof(dctx));
	memset(&keySchedule, 0x0, sizeof(keySchedule));

	if (NULL == buf || NULL == length)
	{
		pub_log_info("[%s][%d] param is error", __FILE__, __LINE__);
		return -1;
	}

	ilen = *length;
	ptr = (char*)calloc(sizeof(char), 2*ilen);
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, 2*ilen, errno, strerror(errno));
		return -1;
	}
	EVP_DecodeInit(&dctx);
	ret = EVP_DecodeUpdate(&dctx, ptr, &olen, buf, ilen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] EVP_DecodeUpdate error! ret=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, ret, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	slen += olen;
	ret = EVP_DecodeFinal(&dctx, ptr + slen, &olen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] EVP_DecodeFinal error! ret=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, ret, errno, strerror(errno));
		free(ptr);
		return -1;
	}
	slen += olen; 

	DES_set_key_unchecked(&keyEncrypt, &keySchedule);
	DES_ncbc_encrypt(ptr, buf, slen, &keySchedule, &ivec, DES_DECRYPT);
	nlen = buf[slen-1];
	buf[slen-nlen] = '\0';
	*length = slen - nlen;
	free(ptr);

	return 0;
}

int tcp_des(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	int	len = 0;
	int	ret = 0;
	char	tmp[32];
	
	memset(tmp, 0x0, sizeof(tmp));
	len = buf->len - HEAD_LEN;
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d] 开始解密...", __FILE__, __LINE__);
		ret = des_dec(buf->data + HEAD_LEN, &len);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] 解密失败!", __FILE__, __LINE__);
			return -1;
		}
		buf->len = len + HEAD_LEN;
		sprintf(tmp, "%0*d", HEAD_LEN, len);
		memcpy(buf->data, tmp, HEAD_LEN);
	
		return 0;
	}

	pub_log_info("[%s][%d] 开始加密...", __FILE__, __LINE__);
	ret = des_enc(buf->data + HEAD_LEN, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] 加密失败!", __FILE__, __LINE__);
		return -1;
	}
	buf->len = len + HEAD_LEN;
	sprintf(tmp, "%0*d", HEAD_LEN, len);
	memcpy(buf->data, tmp, HEAD_LEN);
	
	return 0;
}

