/****************************************************************
**** 数字签名程序，引用openssl相关库函数   
**** author:zhanghailu
**** date  :2014-07-03
*****************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/pkcs12.h"
#include "openssl/bio.h"
#include "openssl/buffer.h"
#include "pub_vars.h"
#include "pub_xml.h"

#define MAX_PACK_SIZE	(1024*5)
#define MAX_SIGN_SIZE (1024*4)

#define PRICERTFILE	"server.pfx"
#define CERTFILE	"allinpay.cer"
#define PRIPASSWD	"Abcd1234"

extern sw_int_t pub_xml_pack_node(sw_xmltree_t *pxml, sw_xmlnode_t *node, char *pack);
extern int pub_vars_pack_node(sw_loc_vars_t *vars, char *name, char *pack);

static char hex2int(unsigned char hex)
{
	hex = hex - '0';
	if (hex > 9)
	{
		hex = (hex + '0' - 1) | 0x20;
		hex = hex - 'a' + 11;
	}
	
	if (hex > 15)
	{
		hex = 0xFF;
	}
		
	return hex;
}

static int urldecode_internal(char *url, sw_int_t *len, int is_query)
{
	char	*dst;
	const char	*src;
	unsigned char	high, low;
	
	if (url == NULL)
	{
		pub_log_error("[%s][%d] url is null!", __FILE__, __LINE__);
		return -1;
	}
	
	src = (const char*)url;
	dst = (char*)url;
	while (*src != '\0')
	{
		if (is_query && *src == '+')
		{
			*dst = ' ';
		}
		else if (*src == '%')
		{
			*dst = '%';
			high = hex2int(*(src + 1));
			if (high != 0xFF) {
				low = hex2int(*(src + 2));
				if (low != 0xFF)
				{
					high = (high << 4) | low;
					if (high < 32 || high == 127)
					{
						high = '_';
					}
					*dst = high;
					src += 2;
				}
			}
		}
		else
		{
			*dst = *src;
		}

		dst++;
		src++;
	}

	*dst = '\0';
	*len = (dst - url) + 1;

	return 0;
}

int urldecode_path(char *url, sw_int_t *len)
{
	return urldecode_internal(url, len, 0);
}

int urldecode_query(char *url, sw_int_t *len)
{
	return urldecode_internal(url, len, 1);
}

static int convert(sw_buf_t *buf, int flag)
{
	int	ret = 0;
	size_t	len = 0;
	size_t	blen = 0;
	size_t	alen = 0;
	size_t	savelen = 0;
	char	*ptr = NULL;
	char	*pa = NULL;
	char	*pb = NULL;

	blen = buf->len;
	
	alen = blen * 2;
	savelen = alen;
	
	pa = (char *)calloc(1, alen + 1);
	if (pa == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, alen + 1, errno, strerror(errno));
		return -1;
	}

	if (flag == 0)
	{
		pub_log_info("[%s][%d] GBK->UTF-8", __FILE__, __LINE__);
		ret = pub_code_g2u(buf->data, &blen, pa, &alen);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] g2u error!", __FILE__, __LINE__);
			free(pa);
			return -1;
		}
	}
	else
	{
		pub_log_info("[%s][%d] UTF-8->GBK", __FILE__, __LINE__);
		ret = pub_code_u2g(buf->data, &blen, pa, &alen);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] u2g error!", __FILE__, __LINE__);
			free(pa);
			return -1;
		}
	}	
	pub_log_bin(SW_LOG_INFO, pa, strlen(pa), "[%s][%d] savelen=[%d] alen=[%d] len=[%d]",
		__FILE__, __LINE__, savelen, alen, strlen(pa));
	
	len = savelen - alen;
	ret = pub_buf_update(buf, len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		free(pa);
		return -1;
	}
	
	memset(buf->data, 0x0, buf->size);
	memcpy(buf->data, pa, len);
	buf->len = len;
	buf->data[buf->len] = '\0';
	
	free(pa);
	pub_log_bin(SW_LOG_INFO, buf->data, buf->len, "[%s][%d] after convert:[%d]", __FILE__, __LINE__, buf->len);
	
	return 0;
}

/************************************************
***desc:base64 encode 
***input :char *message  input message
	  int  len	 input message length
	  int *outlen    input  size of b64_out buffer 
   output:char *b64_out  output  message of base64-encode
   	  int *outlen    output  the length of base64-encode message
return value :  0     success
		false    fail
***author:zhanghailu
***date  :2014-07-03
*************************************************/
static int _base64_encode(char *message, int len, char *b64_out, int *outlen)
{
	BIO	*bio = NULL;
	BIO	*b64 = NULL;
	BIO	*mbio = NULL;
	BUF_MEM	*bptr = NULL;
	
	if (message == NULL || len < 0 || b64_out == NULL || outlen == NULL || *outlen < len)
	{
		pub_log_error("[%s][%d] Params error!", __FILE__, __LINE__);
		return -1;
	}
	
	mbio = BIO_new(BIO_s_mem());
	if (mbio == NULL)
	{
		pub_log_error("[%s][%d] BIO_new error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	bio = BIO_new(BIO_f_base64());
	if (bio == NULL)
	{
		pub_log_error("[%s][%d] BIO_new error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	b64 = BIO_push(bio, mbio);
	if (b64 == NULL)
	{
		pub_log_error("[%s][%d] BIO_push error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	if (BIO_write(b64, message, len) != len)
	{
		pub_log_error("[%s][%d] BIO_write error! errno=[%d]:[%s]", __FILE__, __LINE__, errno, strerror(errno));
		BIO_free_all(b64);
		CRYPTO_cleanup_all_ex_data();
		return -1;
	}
	BIO_flush(b64);

	BIO_get_mem_ptr(b64, &bptr);
	
	if (*outlen < bptr->length)
	{
		pub_log_error("[%s][%d] Invalid params!", __FILE__, __LINE__);
		BIO_free_all(b64);
		CRYPTO_cleanup_all_ex_data();
		return -1;
	}
	memcpy(b64_out, bptr->data, bptr->length);
	b64_out[bptr->length] = '\0';
	*outlen = bptr->length;

	BIO_free_all(b64);
	CRYPTO_cleanup_all_ex_data();
	
	return 0;
}

/************************************************
***desc:base64 decode 
***input :char *b64_in   input b64 encode message
	  int  len	 input message length
	  int *outlen    input  size of out buffer 
   output:char *out      output  message of base64-decode
   	  int *outlen    output  the length of base64-decode message
return value :  0     success
		false    fail
***author:zhanghailu
***date  :2014-07-03
*************************************************/
static int _base64_decode(char *b64_in, int len,char *out, int *outlen)
{
	int	ret = 0;
	BIO	*bio = NULL;
	BIO	*b64 = NULL;
	BIO	*mbio = NULL;
	
	if (b64_in == NULL|| len <= 0 || out == NULL || outlen == NULL ) 
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	b64 = BIO_new(BIO_f_base64());
	if (b64 == NULL)
	{
		pub_log_error("[%s][%d] BIO_new error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	mbio = BIO_new_mem_buf(b64_in, len);
	if (mbio == NULL)
	{
		pub_log_error("[%s][%d] BIO_new_mem_buf error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	bio = BIO_push(b64, mbio);

	ret = BIO_read(bio, out, *outlen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] BIO_read error!", __FILE__, __LINE__);
		BIO_free_all(bio);
		CRYPTO_cleanup_all_ex_data();
		return -1;
	}
	out[ret] = '\0';
	*outlen = ret;

	BIO_free_all(bio);	
	CRYPTO_cleanup_all_ex_data();
	
	return 0;
}

/****************************************************************
**** 解析P12证书文件，结果存储在字符串对象中(PEM编码)       
****
*****************************************************************/
static int _PraseP12CertToMem_PEM(char *p12Cert, char *p12pass, EVP_PKEY **pkey)
{
	FILE	*fp = NULL;
	X509	*x509 = NULL;
	PKCS12	*p12 = NULL;
	STACK_OF(X509)	*ca  = NULL;
	
	if (p12Cert == NULL || p12pass == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	/*读取个人信息证书并分解出密钥和证书*/
	fp = fopen(p12Cert, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] 读取p12证书失败! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	p12 = d2i_PKCS12_fp(fp, NULL);
	if (p12 == NULL) 
	{
		pub_log_error("[%s][%d] Read PKCS12 info error!", __FILE__, __LINE__);
		ERR_print_errors_fp(stderr);
		PKCS12_free(p12);
		fclose (fp);
		return -1;
	}
	fclose (fp);

	if (!PKCS12_parse(p12, p12pass, pkey, &x509, &ca))
	{
		pub_log_error("[%s][%d] Parse PKCS12 error!", __FILE__, __LINE__);
		ERR_print_errors_fp(stderr);
		PKCS12_free(p12);
		return -1;
 	}
	PKCS12_free(p12);
	X509_free(x509);
	
	return 0;
}
/***************************************************************
解析CER证书文件，结果存储在字符串对象中(PEM编码)
****************************************************************/
static int _PraseCertToMem_PEM(char *pCert, EVP_PKEY **pkey)
{
	FILE	*fp = NULL;
	X509	*x509 = NULL;
	
	if (pCert == NULL || pCert[0] == '\0')
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	fp = fopen(pCert, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] 读取CER证书文件失败! errno=[%d]:[%s]", 
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	x509 = PEM_read_X509(fp, NULL, NULL, NULL);
	if (x509 == NULL)
	{
		pub_log_error("[%s][%d] PEM read x509 error!", __FILE__, __LINE__);
		ERR_print_errors_fp(stderr);
		fclose(fp);
		return -1;
	}
	fclose(fp);
 
	/* Get public key - eay */
	*pkey = X509_get_pubkey(x509);
	if (pkey == NULL)
	{
		pub_log_error("[%s][%d] X509 get pubkey error!", __FILE__, __LINE__);
		ERR_print_errors_fp(stderr);
		return -1;
	}
	X509_free(x509);

 	return 0;
}
/**************************************************************************
   desc:  sign 
***************************************************************************/
static int _sign(char *input, int inputLen, char* output, int *outputLen)
{
	int	ret = 0;
	int	len = MAX_SIGN_SIZE;
	char	*ptr = NULL;
	char	keyfile[128];
	EVP_PKEY	*priKey = NULL;
	EVP_MD_CTX	md_ctx;

	if (input== NULL || output == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	ptr = (char *)calloc(1, MAX_SIGN_SIZE);
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	SSLeay_add_all_algorithms();
	ERR_load_crypto_strings();

	memset(keyfile, 0x0, sizeof(keyfile));
	sprintf(keyfile, "%s/cert/%s", getenv("SWWORK"), PRICERTFILE);
	ret = _PraseP12CertToMem_PEM(keyfile, PRIPASSWD, &priKey);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] Read cert error!", __FILE__, __LINE__);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	
	EVP_SignInit(&md_ctx, EVP_sha1());
	EVP_SignUpdate(&md_ctx, input, inputLen);
	ret = EVP_SignFinal(&md_ctx, (char*)ptr, (unsigned int *)&len, priKey);
	if (ret != 1)
	{
		pub_log_error("[%s][%d] EVP_SignFinal error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		ERR_print_errors_fp(stderr);
		EVP_MD_CTX_cleanup(&md_ctx);
		EVP_PKEY_free(priKey);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	ptr[len] = '\0';
	EVP_MD_CTX_cleanup(&md_ctx);

	ret = _base64_encode(ptr, len, output, outputLen);
	if (ret != 0 || *outputLen <= 0)
	{
		pub_log_error("[%s][%d] base64 encode error!", __FILE__, __LINE__);
		EVP_PKEY_free(priKey);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	EVP_PKEY_free(priKey);
	free(ptr);

	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();
	
	return 0;
}

/***********************************************************************
签名验证
************************************************************************/
static int _verify(char *input, int inputLen, char *sign, int signLen)
{
	int	ret = 0;
	int	len = MAX_SIGN_SIZE;
	char	*ptr = NULL;
	char	certfile[128];
	EVP_PKEY	*pcert = NULL;
	EVP_MD_CTX	md_ctx;

	if (input == NULL || inputLen <= 0 || sign == NULL || signLen <= 0)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return -1;
	}

	ptr = (char *)calloc(1, MAX_SIGN_SIZE);
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	SSLeay_add_all_algorithms();
	ERR_load_crypto_strings();

	memset(certfile, 0x0, sizeof(certfile));
	sprintf(certfile, "%s/cert/%s", getenv("SWWORK"), CERTFILE);
	ret = _PraseCertToMem_PEM(certfile, &pcert);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] Read cert error!", __FILE__, __LINE__);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	EVP_SignInit(&md_ctx, EVP_sha1());
	
	ret = _base64_decode(sign, signLen, ptr, &len);
	if (ret != 0 || len <= 0)
	{
		pub_log_error("[%s][%d] Base64 decode error! ret=[%d] len=[%d]", 
			__FILE__, __LINE__, ret, len);
		EVP_MD_CTX_cleanup(&md_ctx); 
		EVP_PKEY_free(pcert);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	ptr[len] = '\0';

	EVP_VerifyUpdate(&md_ctx,input, inputLen);
	ret = EVP_VerifyFinal(&md_ctx, ptr, len, pcert);
	if (ret != 1)
	{
		pub_log_error("[%s][%d] EVP_VerifyFinal error!", __FILE__, __LINE__);
		ERR_print_errors_fp(stderr);
		EVP_MD_CTX_cleanup(&md_ctx); 
		EVP_PKEY_free(pcert);
		free(ptr);
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
		ERR_remove_state(0);
		ERR_free_strings();
		return -1;
	}
	EVP_MD_CTX_cleanup(&md_ctx); 
	EVP_PKEY_free(pcert);
	free(ptr);
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();
	
	return 0;
}

int allinpay_sign(char *input, int inputLen, char *output, int *outputLen)
{
	return _sign(input, inputLen, output, outputLen);
}

int allinpay_verify(char *input, int inputLen, char *sign, int signLen)
{
	return _verify(input, inputLen, sign, signLen);
}

static int zip_0d0a(char *str)
{
	char    *ptr = str;

	while (*str != '\0')
	{
		if (*str == ' ' || *str == '\t' || *str == 0xa || *str == 0xd)
		{
			str++;
			continue;
		}
		*ptr++ = *str++;
	}
	*ptr = '\0';

	return 0;
}

int allinsign(sw_loc_vars_t *vars, sw_buf_t *buf)
{
	int	ret = 0;
	int	len = 0;
	int xmllen = 0;
	char	*pb = NULL;
	char	*pe = NULL;
	sw_buf_t	tmpbuf;
	char	*ptr = NULL;
	char	sign[MAX_SIGN_SIZE];
	sw_int_t	slen = 0;
	sw_int_t	totallen = 0;

	pub_buf_init(&tmpbuf);
	memset(sign, 0x0, sizeof(sign));

	xmllen = xml_pack_length(vars->tree);
	if (xmllen < 0)
	{
		pub_log_error("[%s][%d] Get xml length error!", __FILE__, __LINE__);
		return -1;
	}

	ret = pub_buf_update(&tmpbuf, xmllen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] update error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	} 

	memset(tmpbuf.data, 0x0, tmpbuf.size);
	ret = pub_vars_pack_node(vars, ".RESPONSE.ENVELOPE", tmpbuf.data);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] pack node error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}

	zip_0d0a(tmpbuf.data);
	tmpbuf.len = strlen(tmpbuf.data);
	ret = convert(&tmpbuf, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] convert error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	
	len = sizeof(sign);
	memset(sign, 0x0, sizeof(sign));
	pub_log_info("[%s][%d] signstr=[%s]", __FILE__, __LINE__, tmpbuf.data);
	ret = allinpay_sign(tmpbuf.data, tmpbuf.len, sign, &len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Allipay_sign error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	pub_log_info("[%s][%d] sign=[%s]", __FILE__, __LINE__, sign);
	
	ret = pub_buf_update(buf, xmllen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		return -1;
	}
	
	memset(buf->data, 0x0, buf->size);
	ret = pub_xml_pack_ext(vars->tree, buf->data);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] pub_xml_pack_ext error!", __FILE__, __LINE__);
		return -1;
	}
	
	zip_0d0a(buf->data);
	buf->len = strlen(buf->data);
	buf->data[buf->len] = '\0';
	
	pb = strstr(buf->data, "</RESPONSE>");
	if (pb == NULL)
	{
		pub_log_error("[%s][%d] RESPONSE not found!", __FILE__, __LINE__);
		return -1;
	}
	
	totallen = buf->len + MAX_PACK_SIZE;
	ret = pub_buf_update(&tmpbuf, totallen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] update error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	
	memset(tmpbuf.data, 0x0, tmpbuf.size);
	memcpy(tmpbuf.data, buf->data, pb - buf->data);
	strcat(tmpbuf.data, "<SIGNATURE><SIGN_TYPE>1</SIGN_TYPE><SIGN_MSG>");
	strcat(tmpbuf.data, sign);
	strcat(tmpbuf.data, "</SIGN_MSG></SIGNATURE>");
	strcat(tmpbuf.data, pb);
	tmpbuf.len = strlen(tmpbuf.data);
	slen = tmpbuf.len;
	
	ret = pub_buf_update(buf, slen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	
	memset(buf->data, 0x0, buf->size);
	memcpy(buf->data, tmpbuf.data, slen);
	buf->len = slen;
	buf->data[buf->len] = '\0';
	
	pub_log_bin(SW_LOG_DEBUG, buf->data, buf->len, "[%s][%d] conv before:[%d]", __FILE__, __LINE__, buf->len);
	ret = convert(buf, 0);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] convert error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	slen = buf->len;

	ret = pub_buf_update(&tmpbuf, slen * 2);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		pub_buf_clear(&tmpbuf);
		return -1;
	}
	
	totallen = 0;
	ret = pub_code_base64_enc(buf->data, tmpbuf.data, slen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Base64 enc error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}

	totallen = ret;
	ret = pub_buf_update(buf, totallen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}
	
	memcpy(buf->data, tmpbuf.data, totallen);
	buf->len = totallen;
	buf->data[buf->len] = '\0';
	
	pub_log_info("[%s][%d] Base64 enc:[%s][%d]", __FILE__, __LINE__, buf->data, buf->len);
	
	return 0;
}

int allinverify(sw_loc_vars_t *vars, sw_buf_t *buf)
{
	int	len = 0;
	int	ret = 0;
	int	xmlheadlen = 0;
	int totallen = 0;
	sw_int_t	elen = 0;
	char	*ptr = NULL;
	char	*pb = NULL;
	char	*pe = NULL;
	char	xmlhead[128];
	char	sign[MAX_SIGN_SIZE];
	sw_buf_t	locbuf;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	
	memset(sign, 0x0, sizeof(sign));
	
	if (strncmp(buf->data, "reqMsg=", 7) == 0)
	{
		memset(&locbuf, 0x0, sizeof(locbuf));
		pub_buf_init(&locbuf);
		if (pub_buf_chksize(&locbuf, buf->size) != SW_OK)
		{
			pub_log_error("[%s][%d] Update buffer error!", __FILE__, __LINE__);
			pub_buf_clear(&locbuf);
			return -1;
		}
		memcpy(locbuf.data, buf->data + 7, buf->len - 7);
		locbuf.len = buf->len - 7;
		memset(buf->data, 0x0, buf->size);
		memcpy(buf->data, locbuf.data, locbuf.len);
		buf->len = locbuf.len;
		pub_buf_clear(&locbuf);
	}
	pub_log_bin(SW_LOG_INFO, buf->data, buf->len, "[%s][%d] before decode:[%d]", __FILE__, __LINE__, buf->len);
	urldecode_path(buf->data, &buf->len);
	pub_log_bin(SW_LOG_INFO, buf->data, buf->len, "[%s][%d] after decode:[%d]", __FILE__, __LINE__, buf->len);
	
	totallen =  buf->len;
	ptr = (char *)calloc(totallen, sizeof(char));
	if (ptr == NULL)
	{
		pub_log_error("[%s][%d] Calloc error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}
	
	pub_log_bin(SW_LOG_INFO, buf->data, buf->len, "[%s][%d] Pack:[%d]", __FILE__, __LINE__, buf->len);
	
	elen = 0;
	ret = pub_code_base64_dec(buf->data, ptr, &elen);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Base64 dec error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}
	
	len = elen;
	pub_log_bin(SW_LOG_INFO, ptr, len, "[%s][%d] Base64 dec:[%d]", __FILE__, __LINE__, len);
		
	memset(buf->data, 0x0, buf->size);
	buf->len = 0;
	if (ptr[0] == '<' && strncmp(ptr, "<?xml", 5) != 0)
	{
		memset(xmlhead, 0x0, sizeof(xmlhead));
		sprintf(xmlhead, "%s", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n");
		xmlheadlen = strlen(xmlhead);
			
		memset(buf->data, 0x0, buf->size);
		memcpy(buf->data, xmlhead, xmlheadlen);
		buf->len += xmlheadlen;
	}
	
	ret = pub_buf_chksize(buf, len);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}
	
	memcpy(buf->data + buf->len, ptr, len);
	buf->len += len;
	
	buf->data[buf->len] = '\0';

	ret = convert(buf, 1);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] convert error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}

	pb = strstr(ptr, "<SIGN_MSG>");
	if (pb == NULL)
	{
		pub_log_error("[%s][%d] SIGN_MSG not found!", __FILE__, __LINE__);
		return -1;
	}

	pe = strstr(pb, "</SIGN_MSG>");
	if (pe == NULL)
	{
		pub_log_error("[%s][%d] SIGN_MSG not found!", __FILE__, __LINE__);
		return -1;
	}
	
	memcpy(sign, pb + 10, pe - pb -10);
	pub_log_info("[%s][%d] sign=[%s]", __FILE__, __LINE__, sign);

	xml = pub_xml_unpack_ext(ptr, len);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Xml unpack error!", __FILE__, __LINE__);
		free(ptr);
		return -1;
	}
	
	memset(ptr, 0x0, totallen);
	node = pub_xml_locnode(xml, ".REQUEST.ENVELOPE");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] .REQUEST.ENVELOPE not found!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		free(ptr);
		return -1;
	}

	ret = pub_xml_pack_node(xml, node, ptr);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] Pack node error!", __FILE__, __LINE__);
		pub_xml_deltree(xml);
		free(ptr);
		return -1;
	}
	pub_xml_deltree(xml);
	zip_0d0a(ptr);
	pub_log_info("[%s][%d] signstr=[%s]", __FILE__, __LINE__, ptr);

	ret = allinpay_verify(ptr, strlen(ptr), sign, strlen(sign));
	if (ret < 0)
	{
		pub_log_error("[%s][%d] allinpay_verify error!", __FILE__, __LINE__);
		loc_set_zd_data(vars, "#chkerr", "1");
	}
	else
	{
		pub_log_info("[%s][%d] verify success !!!!", __FILE__, __LINE__);
	}
	free(ptr);

	return 0;
}

int allinpaysecure(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	if (flag == 0)
	{
		return allinsign(vars, buf);
	}
	
	return allinverify(vars, buf);
}

