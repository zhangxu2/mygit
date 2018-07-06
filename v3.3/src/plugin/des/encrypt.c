#include "pub_log.h"
#include "pub_type.h"
#include "pub_vars.h"
#include "pub_buf.h"

/****�������Ļ����ӽ��ܻ�������****/

static int tcp_dec(char *buf, sw_int_t *length);
static int tcp_enc(char *buf, sw_int_t *length);
static int tux_enc(char *buf, sw_int_t *length);
static int tux_dec(char *buf, sw_int_t *length);
static int packencrypt(char *str, sw_int_t len);
static int packdecrypt(char *str, sw_int_t len);
static int apatoi(char *s,int len);

int tcp_des(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d] ��ʼ����...", __FILE__, __LINE__);
		return tcp_dec(buf->data, &buf->len);
	}
	
	pub_log_info("[%s][%d] ��ʼ����...", __FILE__, __LINE__);
	return tcp_enc(buf->data, &buf->len);
}

int tcp_dec(char *buf, sw_int_t *length)
{
	char	*str = buf;
	int	head_len = 68;
	int	len = *length;
	
	pub_log_info("[%s][%d] ���ڽ���..., len=[%d]", __FILE__, __LINE__, len);

	if (len - head_len < 0)
	{
		pub_log_error("[%s][%d] ���ĳ�������! len=[%d]", __FILE__, __LINE__, len);
		return -1;
	}

	if (len == (head_len + 4) || (str[head_len] == 'S' && str[head_len + 4] == '\0') || (u_char)str[head_len + 4] == 0xff)
	{
		/****ֻ��4λӦ��������,�����һ������8583�ṹ�ı���****/
		char	reply[4];
		memset(reply, 0x0, sizeof(reply));

		pub_log_info("Ӧ����ֻ��4λ��Ӧ����[%.4s]\n", str + head_len);
		memcpy(reply, str + head_len, 4);
		strcpy(str + head_len, "30001");
		memcpy(str + 5 + head_len,"\x80\x10",2);
		memset(str + 7 + head_len,'\0', 14);
		memcpy(str + 21 + head_len, reply, 4);
		str[25 + head_len] = '\0';
		len = 25 + head_len;
		*length = len;
		return 0;
	}
	len = apatoi(buf, 8) + 8;
	packdecrypt(buf + head_len, len - head_len);
	*length = len;
	pub_log_bin(SW_LOG_DEBUG, buf, *length, "���ܴ���� len=[%d]", __FILE__, __LINE__, *length);

	return 0;
}


int tcp_enc(char *buf, sw_int_t *length)
{
	int	head_len = 68;
  	int	len = *length;

	pub_log_info("�����ڼ��ܺ����ڲ�[%s][%d] len=[%d]", __FILE__, __LINE__,len);
	if (len - head_len < 0)
	{
		pub_log_info("���ĳ��ȴ���[%s][%d] len=[%d]\n", __FILE__, __LINE__, len);
		return -1;
	}

	len = apatoi(buf, 8) + 8;
	packencrypt(buf + head_len,len - head_len);
	pub_log_bin(SW_LOG_DEBUG, buf, *length,"���ܴ����[%s][%d] len=[%d]", __FILE__, __LINE__, *length);

    	return 0;
}


/*����*/
int packdecrypt(char *str, sw_int_t len)
{
  	int	i = 0;
	int	klen = 0;
	char	key[33];

	memset(key, 0x0, sizeof(key));

	strcpy(key, "dhccnew");
	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		str[i] = str[i] - key[i % klen];
	}
	pub_log_info("[%s][%d] ���ܽ���...", __FILE__, __LINE__);

	return 0;
}

/*����*/
int packencrypt(char *str, sw_int_t len)
{
	int	i = 0;
	int	klen = 0;
	char	key[33];

	memset(key, 0x0, sizeof(key));
	strcpy(key, "dhccnew");

	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		str[i] = str[i] + key[i % klen];

	}
	pub_log_info("[%s][%d],���ܽ���...", __FILE__, __LINE__);
    	return 0;
}

/**
������:apatoi
����  :��ָ�������ַ���ת��������
����  :
       s                �����ַ���
       len              ����
����ֵ:
       ת���������
**/
int apatoi(char *s,int len){
	int i;
	int weight=1;
	int n;
	int sum;

	if((s[0]>'9' ||s[0]<'0') && s[0]!=' '){
		return(0);
	}
	sum=0;
	for(i=len-1;i>=0;i--){
		if(s[i]>'9'||s[i]<'0'){
			n=0;
		}else{
			n=s[i]-'0';
			sum+=n*weight;
			weight*=10;
		}
	}
	return(sum);
}


int tux_des(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	if (flag == SW_DEC)
	{
		pub_log_info("[%s][%d],������ܺ���...,,", __FILE__, __LINE__);
		return tux_dec(buf->data, &buf->len);
	}

	pub_log_info("[%s][%d] ������ܺ���.....", __FILE__, __LINE__);
	return tux_enc(buf->data, &buf->len);
}

/**
������:tux_dec
����  :�������Ľ��ܺ���(�������)
����  :
       ��
����ֵ:0/-1
ʹ��˵��:
**/
int tux_dec(char *buf, sw_int_t *length)
{
	int	head_len = 60;
	char	*str = buf;
	int	len = *length;

	pub_log_info("������TUX���ܺ����ڲ�[%s][%d] len=[%d]\n", __FILE__, __LINE__,len);
	if (len - head_len < 0)
	{
		pub_log_info("���ĳ��ȴ���[%s][%d] len=[%d]\n", __FILE__, __LINE__, len);
		return -1;
	}

	if (len == (head_len + 4) || (str[head_len] == 'S' && str[head_len + 4] == '\0') || (u_char)str[head_len + 4] == 0xff)
	{
		/****ֻ��4λӦ��������,�����һ������8583�ṹ�ı���****/
		char	reply[4];
		memset(reply,'\0',sizeof(reply));

		pub_log_info("Ӧ����ֻ��4λ��Ӧ����[%.4s]\n", str + head_len);
		memcpy(reply, str + head_len, 4);
		strcpy(str + head_len, "30001");
		memcpy(str + 5 + head_len, "\x80\x10", 2);
		memset(str + 7 + head_len, '\0', 14);
		memcpy(str + 21 + head_len, reply, 4);
		str[25 + head_len] = '\0';
		len = 25 + head_len;
		*length = len;
		pub_log_bin(SW_LOG_DEBUG, buf, *length, "TUX���ܴ����[%s][%d]\n", __FILE__, __LINE__);
		return 0;
	}
	packdecrypt(buf + head_len, *length - head_len);
	pub_log_bin(SW_LOG_DEBUG, buf, *length, "TUX���ܴ����[%s][%d]\n", __FILE__, __LINE__);

	return 0;
}

/**
������:tux_enc
����  :�������ļ��ܺ���(�������)
����  :
       ��
����ֵ:0/-1
ʹ��˵��:
**/
int tux_enc(char *buf, sw_int_t *length)
{
	int	head_len = 60;
	int	len = *length;

	pub_log_info(buf, *length, "���ܴ���ǰ[%s][%d],len=[%ld]\n", __FILE__, __LINE__, *length);
	if (len - head_len < 0)
	{
		pub_log_info("���ĳ��ȴ���[%s][%d] len=[%d]\n", __FILE__, __LINE__, len);
		return -1;
	}
	packencrypt(buf + head_len, *length - head_len);
	pub_log_bin(SW_LOG_DEBUG, buf, *length, "TUX���ܴ����[%s][%d],len=[%ld]\n", __FILE__, __LINE__, *length);

	return 0;
}
