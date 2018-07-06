/*********************************************************************
 *** �� �� ��: v2.0
 *** ��������: liteng
 *** ��������: 2011-11-18
 *** ����ģ��: ����ģ��
 *** ��������: pub_buf.c
 *** ��������: ������ƽ̨����������
 *** �����б�: 
 ***		pub_buf_init	ƽ̨���Ļ�������ʼ��
 ***		pub_buf_update	ƽ̨���Ļ�������С����
 ***		pub_buf_clear	ƽ̨���Ļ��������
 *** ʹ��ע��:	
 *** �޸ļ�¼:
 *** 	�޸�����:
 *** 	�޸�ʱ��:
 *** 	�޸�����:
 ********************************************************************/
#include "pub_buf.h"

/******************************************************************************
 *** ��������: pub_buf_init
 *** ��������: ƽ̨���Ļ�������ʼ��
 *** ��������: 
 *** ��������: 2011-11-18 13:57
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: buf	ƽ̨������ָ��
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
int pub_buf_init(sw_buf_t *buf)
{
	if (buf == NULL)
	{
		return -1;
	}
	
	memset(buf,0x00,sizeof(sw_buf_t));
	buf->len = 0;
	buf->size = BUF_MAXLEN;
	buf->data = NULL;
	buf->data = (char*)malloc(buf->size*sizeof(char));
	if (buf->data == NULL)
	{
		pub_log_error("[%s][%d] �ڴ����ʧ��! errno:%d",
			__FILE__, __LINE__,errno);
		return -1;
	}
	
	memset(buf->data,0x00,buf->size);
	return 0;
}


/******************************************************************************
 *** ��������: pub_buf_update
 *** ��������: ƽ̨���Ļ�������С����
 *** ��������: 
 *** ��������: 2011-11-18 13:59
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: buf	ƽ̨������ָ��
 *** 	����1: iSize		���º�Ĵ�С
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
int pub_buf_update(sw_buf_t *buf, sw_int_t iSize)
{
	if (NULL == buf)
	{
		return -1;
	}
	
	if (iSize <= 0)
	{
		buf->size = buf->size * 2 + 8;
	}
	else
	{
		buf->size = iSize + 2;
	}

	if (NULL != buf->data)
	{
        	free(buf->data);
		buf->data = NULL;
	}

	buf->data = (char*)malloc(buf->size);	
	if (NULL == buf->data)
	{
		return -1;
	}
	memset(buf->data, 0, buf->size);
	
	return 0;
}

/******************************************************************************
 **��������: pub_base_buf_chksize
 **��    ��: ��黺��ռ��Ƿ��㹻
 **�������:
 **      buf: ƽ̨���Ļ�����
 **      iSize: ����Ŀռ��С
 **�������:
 **��    ��: true: �ռ��㹻 false: �ռ�����ʧ��
 **ʵ������: 
 **     1. ʣ�໺��ռ䲻��ʱ������ռ�
 **     2. ʣ�໺��ռ��㹻ʱ����������
 **ע������: 
 **��    ��: # Qifeng.zou # 2012.09.11 #
 ******************************************************************************/
sw_int_t pub_buf_chksize(sw_buf_t *buf, size_t size)
{
	size_t left = 0;
	size_t mod = 0;
	size_t block = 0;
	size_t orilen;
	char *psOriBuf;

	if(buf->len < 0)
	{
		return SW_ERR;
	}

	orilen = buf->len;
	left = buf->size - orilen;
	if(left <= size)
	{

		block = size / SW_BUFF_INCR_SIZE;
		mod = size % SW_BUFF_INCR_SIZE;
		if(mod >= left)
		{
			block++;
		}
		buf->size += block * SW_BUFF_INCR_SIZE;

		psOriBuf = NULL;
		psOriBuf = (char*)malloc((orilen+1)*sizeof(char));
		if(NULL == psOriBuf)
		{
			pub_log_error("%s[%d] �ڴ����ʧ��! errno:%d",
				__FILE__, __LINE__, errno);
			return SW_ERR;
		}
		memset(psOriBuf,0x00,orilen+1);
		memcpy(psOriBuf,buf->data,orilen);

		if(pub_buf_update(buf,buf->size)<0)
		{
			pub_log_error("%s[%d] �ڴ����ʧ��! errno:%d",
				__FILE__, __LINE__, errno);
			free(psOriBuf);
			psOriBuf=NULL;
			return SW_ERR;
		}
		memcpy(buf->data,psOriBuf,orilen);
		buf->len = orilen;
		free(psOriBuf);
		psOriBuf=NULL;
		pub_log_info("[%s][%d],realloc success,[%d][%d][%d]",__FILE__,__LINE__, size,left,buf->size);
	}
	return SW_OK;
}

/******************************************************************************
 *** ��������: pub_buf_clear
 *** ��������: ƽ̨���Ļ��������
 *** ��������: 
 *** ��������: 2011-11-18 14:1
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: buf	ƽ̨������ָ��
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
int pub_buf_clear(sw_buf_t *buf)
{

	if (buf == NULL)
	{
		return -1;
	}
	
	if (buf->data != NULL)
	{
		free(buf->data);
	}
	
	buf->data = NULL;
	buf->len = 0;
	buf->size = BUF_MAXLEN;
	return 0;
}

static int buf_expand(sw_buf_t *buf, size_t nsize)
{
	char	*tmpbuf = NULL;
	size_t	msize = BUF_MAXLEN;
	
	while (msize < nsize)
	{
		msize <<= 1;
	}
	
	tmpbuf = realloc(buf->data, msize);
	if (tmpbuf == NULL)
	{
		return -1;
	}
	
	buf->data = tmpbuf;
	buf->size = msize;
	
	return 0;
}

sw_buf_t *buf_new()
{
	sw_buf_t	*buf = NULL;
	
	buf = (sw_buf_t *)calloc(1, sizeof(sw_buf_t));
	if (buf)
	{
		buf->data = (char *)calloc(1, BUF_MAXLEN);
		if (buf->data == NULL)
		{
			free(buf);
			return NULL;
		}
		buf->size = BUF_MAXLEN;
		buf->len = 0;
	}
	
	return buf;
}

sw_buf_t *buf_new_string(char *data, int len)
{
	int	size = BUF_MAXLEN;
	sw_buf_t	*buf = NULL;
	
	while (len > size)
	{
		size <<= 1;
	}
	
	buf = (sw_buf_t *)calloc(1, sizeof(sw_buf_t));
	if (buf)
	{
		buf->data = (char *)calloc(1, size);
		if (buf->data == NULL)
		{
			free(buf);
			return NULL;
		}
		memcpy(buf->data, data, len);
		buf->size = size;
		buf->len = len;
	}
	
	return buf;
}

int buf_refresh(sw_buf_t *buf)
{
	if (buf->size > BUF_MAXLEN && buf_expand(buf, BUF_MAXLEN) != 0)
	{
		return -1;
	}
	
	buf->len = 0;
	memset(buf->data, 0x0, buf->size);
	
	return 0;
}

int buf_append(sw_buf_t *buf, char *data, int len)
{
	if (buf->size - buf->len <= len)
	{
		if (buf_expand(buf, buf->size + len) != 0)
		{
			return -1;
		}
	}
	
	memcpy(buf->data + buf->len, data, len);
	buf->len += len;
	
	return 0;
}

int buf_format_append(sw_buf_t *buf, char *fmt, ...)
{
	int	len = 0;
	va_list	ap;
	static char	bufer[8192];
	
	va_start(ap, fmt);
	vsnprintf(bufer, sizeof(bufer), fmt, ap);
	va_end(ap);
	
	len = strlen(bufer);
	if (buf_append(buf, bufer, len) != 0)
	{
		return -1;
	}
	
	return 0;
}

int buf_checksize(sw_buf_t *buf, int len)
{
        if (buf->size - buf->len <= len)
        {
                if (buf_expand(buf, buf->size + len) != 0)
                {
                        return -1;
                }
        }
	
	return 0;
}

int buf_update_string(sw_buf_t *buf, char *data, int len)
{
	if ((buf->size <= len || (buf->size >> 1) > len) && buf_expand(buf, len) != 0)
	{
		return -1;
	}
	
	memcpy(buf->data, data, len);
	buf->data[len] = '\0';
	buf->len = len;
	
	return 0;
}

void buf_release(sw_buf_t *buf)
{
	if (buf->data)
	{
		free(buf->data);
	}
	free(buf);
	
	return ;
}

char *buf_string(sw_buf_t *buf)
{
	return buf->data;
}

int buf_length(sw_buf_t *buf)
{
	return buf->len;
}

