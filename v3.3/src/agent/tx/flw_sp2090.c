/*************************************************
  文 件 名:  flw_sp2090.c                       **
  功能描述:  登陆注册                           **
  作    者:  薛辉                               **
  完成日期:  20160803                           **
 *************************************************/
#include "agent_comm.h"
#include "agent_md5.h"
#include "des3.h"

static int packencrypt_ex(char *string, int len)
{
	int	i = 0;
	int	klen = 0;
	char	key[33];

	pub_mem_memzero(key, sizeof(key));
	strcpy(key, "defineomp");
	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		string[i] = string[i] + key[i % klen];
	}

	return 0;
}

static int generate_key(char *mac, int len, char *key)
{
	int	i = 0;
	int	j = 0;
	unsigned char	tmp[64];

	if (mac == NULL || key == NULL)
	{
		pub_log_debug("err: %s,%d 走到-==================================。\n", __FILE__, __LINE__);
		pub_log_error("err:%s,%d, 参数错误。\n", __FILE__, __LINE__);
		return -1;
	}
	packencrypt_ex(mac, len);
	iMd5ToCh16(mac, tmp, strlen(mac));

	j = 0;
	for (i = 0; i < 16; i++)
	{
		if (i % 4 == 0 && i != 0)
		{
			key[j++] = '-';
		}

		key[j++] = tmp[i];
	}
	return 0;
}

typedef void (*sighandler_t)(int);

/*解密*/
static int packdecrypt(char *string, int len)
{
	int	i = 0;
	int	klen = 0;
	char	key[33];

	pub_mem_memzero(key, sizeof(key));
	strcpy(key, "dhccnew");
	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		string[i] = string[i] - key[i % klen];
	}

	return 0;
}

static int packencrypt(char *string, int len)
{
	int	i = 0;
	int	klen = 0;
	char	key[33];

	pub_mem_memzero(key, sizeof(key));
	strcpy(key, "dhccnew");
	klen = strlen(key);

	for (i = 0; i < len; i++)
	{
		string[i] = string[i] + key[i % klen];
	}

	return 0;
}

static int rannum(char *key, char *res_msg, char *randnum)
{
	int	count = 0;
	u_char	enc_iv[24] = {0, 00, 00, 00, 00, 00, 00, 00,};
	u_char	cipher[16] = {0};
	u_char	keyss[32] = {"asdfghjklzxcvbnmqwertyui"};
	des3_context	ctx3_test;

	if (0 == pub_str_strlen(randnum))
	{
		pub_mem_memzero(res_msg, sizeof(res_msg));
		strncpy(res_msg, "随机码为空 !", sizeof(res_msg) - 1);
		pub_log_debug("[%s][%d] randnum is NULL!\n", __FILE__, __LINE__);
		return -1;
	}

	des3_set_3keys(&ctx3_test, (unsigned char *)keyss);
	des3_cbc_encrypt(&ctx3_test, enc_iv, (unsigned char *)randnum, cipher, 8);
	for (; count < 8; ++count)
	{
		sprintf(key, "%s%02X", key, (unsigned char)cipher[count]);
	}

	return 0;
}

static int get_aixmac(char *mac)
{
	FILE	*fp = NULL;
	char	tmp[1024];

	fp = popen("uname -m", "r");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] popen error!\n", __FILE__, __LINE__);
		return -1;
	}

	while(fgets(tmp, sizeof(tmp) - 1, fp) != NULL)
	{
		strcpy(mac, tmp);
		break;
	}

	pclose(fp);
	mac[strlen(mac) - 1] = '\0';
	return 0;
}

static int get_key(char *mac, char *user, char *passwd, char *role, char *dstkey)
{
	int	len= 0;
	char	buf[256];
	char	key[256];

	if (mac == NULL || user == NULL || passwd == NULL || role == NULL)
	{
		pub_log_error("[%s][%d] 生成密钥失败", __FILE__, __LINE__);
		return -1;	
	}

	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "%s|%s|%s|%s", mac, user, passwd, role);
	len = strlen(buf);
	if (buf[len - 1] == '\n') 
	{
		buf[len - 1] = '\0';
	}

	len = strlen(buf);
	memset(key, 0x00, sizeof(key));
	generate_key(buf, len, key);
	strcpy(dstkey, key);

	return 0;
}

static int my_register(FILE *fp, char *mac, char *authid, char *user, char *passwd, char *res_msg)
{
	int	ret = -1;
	int	len = 0;
	char	mactmp[256];
	char	role[4];
	char	key[256];
	char	buf[256];

	memset(res_msg, 0x00, sizeof(res_msg));
	memset(mactmp, 0x00, strlen(mactmp));
	ret = get_aixmac(mactmp);
	if (ret != 0 )
	{
		pub_log_error("[%s][%d] 注册失败!", __FILE__, __LINE__);
		return -1;
	}

	memset(role, 0x00, sizeof(role));
	memset(key, 0x00, sizeof(key));
	len = strlen(authid);
	strcpy(role, authid + len - 2);	
	ret = get_key(mactmp, user, passwd, role, key);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] 注册失败!", __FILE__, __LINE__);
		return -1;	
	}

	if (strncmp(key, authid, strlen(key)) != 0)
	{
		pub_log_error("[%s][%d]请检查登录密钥是否正确", __FILE__, __LINE__);
		strcpy(res_msg, "请检查登录密钥是否正确!");
		return -1;	
	}

	memset(key, 0x00, sizeof(key));
	ret = get_key(mac, user, passwd, role, key);
	if (ret != 0)
	{
		pub_log_error("[%s][%d] 注册失败!", __FILE__, __LINE__);
		return -1;	
	}

	pub_log_debug("[%s][%d]key2=[%s]", __FILE__, __LINE__, key);		
	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "%s|%s|%s|%s|%s|", key, mac, user, passwd, role);
	len = strlen(buf);
	packencrypt(buf, len);
	fprintf(fp, "%s\n", buf);

	return 0;
}

static int login(FILE *fp, char *res_msg, char *mac, char *user, char *passwd, char *flag)
{
	int	len = 0;
	int	ret = -1;
	char	buf[256];
	char	mactmp[256];
	char	file_user[256];
	char	file_passwd[256];
	char	key[256];
	char	file_key[256];
	char	role[4];
	char	*ptmp = NULL;

	while (!feof(fp))
	{
		memset(buf, 0x00, sizeof(buf));
		fgets(buf, sizeof(buf) - 1, fp);
		pub_log_debug("[%s][%d]the file is [%s]", __FILE__, __LINE__, buf);
		if (strlen(buf) == 0 || buf[0] == '\0' || memcmp(buf, "continue", 8) == 0)
		{
			continue;
		}

		len = strlen(buf);
		if (buf[len - 1] == '\n')
		{
			buf[len - 1] = '\0';
		}

		packdecrypt(buf, len);
		pub_log_debug("[%s][%d]the file buf is [%s]", __FILE__, __LINE__, buf);

		memset(file_key, 0x00, sizeof(file_key));
		ptmp = NULL;
		ptmp = strtok(buf, "|");
		if (ptmp == NULL || strlen(ptmp) == 0 || strncmp(ptmp, "continue", 8) == 0)
		{
			continue;
		}
		memcpy(file_key, ptmp, sizeof(file_key) - 1);

		memset(mactmp, 0x00, sizeof(mactmp));
		ptmp = NULL;
		ptmp = strtok(NULL, "|");
		if (ptmp == NULL || strlen(ptmp) == 0)
		{
			continue;
		}
		memcpy(mactmp, ptmp, sizeof(mactmp) -1);

		memset(file_user, 0x00, sizeof(file_user));
		ptmp = NULL;
		ptmp = strtok(NULL, "|");
		if (ptmp == NULL || strlen(ptmp) == 0)
		{
			continue;
		}
		memcpy(file_user, ptmp, sizeof(file_user) -1);

		memset(file_passwd, 0x00, sizeof(file_passwd));
		ptmp = NULL;
		ptmp = strtok(NULL, "|");
		if (ptmp == NULL || strlen(ptmp) == 0)
		{
			continue;
		}
		memcpy(file_passwd, ptmp, sizeof(file_passwd) -1);

		memset(role, 0x00, sizeof(role));
		ptmp = NULL;
		ptmp = strtok(NULL, "|");
		if (ptmp == NULL || strlen(ptmp) == 0)
		{
			continue;
		}
		memcpy(role, ptmp, sizeof(role) -1);

		len = strlen(role);
		if (role[len - 1] == '\n')
		{
			role[len - 1] = '\0';
		}

		if (strcmp(user, file_user) == 0)
		{
			if (strcmp(passwd, file_passwd) == 0)
			{
				memset(key,0x00,strlen(key));
				ret = get_key(mac, user, passwd, role, key);
				if (ret != 0)
				{
					pub_log_error("[%s][%d] 注册失败!", __FILE__, __LINE__);
					return -1;	
				}
				pub_log_debug("[%s][%d] sfile=[%s] skey=[%s] role=[%s]", __FILE__, __LINE__, file_key, key, role);
				if (strcmp(file_key, key) == 0)
				{
					return 1;	
				}
				else
				{
					pub_log_error("[%s][%d]此用户名已与其他电脑绑定!", __FILE__, __LINE__);
					strcpy(res_msg, "此用户名已与其他电脑绑定!");
					return -1;	
				}
			}
			else
			{
				pub_log_error("[%s][%d]密码输入错误!", __FILE__, __LINE__);
				strcpy(res_msg, "密码输入错误!");
				return -1;		
			}	
		}
	}	

	return 0;
}

int sp2090(sw_loc_vars_t *vars)
{
	int	ret = 0;
	int	len = 0;
	char	flag[4];
	char	opt[8];
	char    reply[8];
	char	buf[1024 * 10];
	char	mac[512];
	char	authid[256];
	char	user[256];
	char	passwd[256];
	char	res_msg[256];
	char	randnum[256];
	char	filename[256];
	char	role[256];
	FILE	*fp=NULL;

	pub_mem_memzero(reply, sizeof(reply));
	pub_mem_memzero(mac, sizeof(mac));
	pub_mem_memzero(authid, sizeof(authid));
	pub_mem_memzero(user, sizeof(user));
	pub_mem_memzero(passwd, sizeof(passwd));
	pub_mem_memzero(randnum, sizeof(randnum));
	pub_mem_memzero(opt, sizeof(opt));
	pub_mem_memzero(flag, sizeof(flag));

	loc_get_zd_data(vars, ".TradeRecord.Request.Opt", opt);
	if (0 == pub_str_strlen(opt))
	{
		strcpy(reply, "E012");
		pub_log_debug("[%s][%d] opt is NULL!\n", __FILE__, __LINE__);
		goto ErrExit;
	}
	pub_log_debug("[%s][%d] opt : [%s] \n", __FILE__, __LINE__, opt);

	if (0 == strcmp(opt, "AUT"))
	{	
		loc_get_zd_data(vars, ".TradeRecord.Request.MacId", mac);
		loc_get_zd_data(vars, ".TradeRecord.Request.AuthId", authid);
		loc_get_zd_data(vars, ".TradeRecord.Request.USER", user);
		loc_get_zd_data(vars, ".TradeRecord.Request.PassWd", passwd);
		loc_get_zd_data(vars, ".TradeRecord.Request.RandNum", randnum);
		loc_get_zd_data(vars, ".TradeRecord.Request.Flage", flag);

		pub_log_debug("[%s][%d]mac[%s]", __FILE__, __LINE__, mac);
		pub_log_debug("[%s][%d]authid[%s]", __FILE__, __LINE__, authid);
		pub_log_debug("[%s][%d]user[%s]", __FILE__, __LINE__, user);
		pub_log_debug("[%s][%d]passwd[%s]", __FILE__, __LINE__, passwd);
		pub_log_debug("[%s][%d]randnum[%s]", __FILE__, __LINE__, randnum);
		pub_log_debug("[%s][%d]flag[%s]", __FILE__, __LINE__, flag);

		if (0 == pub_str_strlen(mac))
		{
			pub_log_info("[%s][%d] mac is NULL! use the default mac\n", __FILE__, __LINE__);
			strncpy(mac, "AAAAAAAAAAAAAAAAAAAAAAAAAAAA", sizeof(mac) - 1);
		}	

		pub_log_debug("[%s][%d] 准备打开文件!\n", __FILE__, __LINE__);

		memset(filename, 0x00, sizeof(filename));
		sprintf(filename, "%s/tmp/.hide.txt", getenv("SWWORK"));

		ret = access(filename, F_OK);
		if (ret)
		{
			fp = fopen(filename, "a+");
			if (fp == NULL)
			{
				pub_log_error("[%s][%d]open file[%s]failed, errno=[%d], [%s]", __FILE__, __LINE__, filename, errno, strerror(errno));
				strcpy(reply, "E016");
				goto ErrExit;
			}
			pub_mem_memzero(res_msg, sizeof(res_msg));
			ret = my_register(fp, mac, authid, user, passwd, res_msg);
			if (ret != 0)
			{
				pub_log_debug("[%s][%d]注册失败!", __FILE__, __LINE__);
				fclose(fp);
				strcpy(reply, "E999");
				goto ErrExit;	
			}

			sprintf(res_msg, "注册成功");
			fclose(fp);
			goto OkExit;
		}

		fp = fopen(filename,"r+");
		if (fp == NULL)
		{
			pub_log_error("[%s][%d]open file[%s]failed, errno=[%d], [%s]", __FILE__, __LINE__, filename, errno, strerror(errno));

			strcpy(reply, "E054");
			sprintf(res_msg, "请检查登录密钥是否正确[%s]", filename);
			goto ErrExit;
		}

		fseek(fp, 0L, 0);
		if (!feof(fp))
		{
			pub_mem_memzero(res_msg, sizeof(res_msg));
			ret = login(fp, res_msg, mac, user, passwd, flag);
			if (ret == -1)
			{
				pub_log_debug("[%s][%d]登录失败!",__FILE__,__LINE__);
				strcpy(reply, "E054");
				fclose(fp);
				goto ErrExit;	
			}
			if (ret == 1)
			{
				goto OkExit;	
			}
			if (ret == 0)
			{
				ret = my_register(fp, mac, authid, user, passwd, res_msg);
				if (ret != 0)
				{
					pub_log_debug("[%s][%d]注册失败!",__FILE__,__LINE__);
					strcpy(reply, "E055");
					fclose(fp);
					goto ErrExit;	
				}
				pub_mem_memzero(res_msg, sizeof(res_msg));
				sprintf(res_msg, "注册成功");
			}
		}	

		fclose(fp);

	}
	else if (0 == strcmp(opt, "UPD"))
	{
		char	file_user[256];
		char	file_passwd[256];
		char	file_key[256];
		char	filename[128];
		char	newpasswd[256];  
		char	tmp[256];
		char	*ptmp=NULL;
		char	mactmp[256];
		char	key[256];

		pub_mem_memzero(newpasswd, sizeof(newpasswd));

		loc_get_zd_data(vars, ".TradeRecord.Request.NEWPassWd", newpasswd);
		loc_get_zd_data(vars, ".TradeRecord.Request.USER", user);
		loc_get_zd_data(vars, ".TradeRecord.Request.PassWd", passwd);
		loc_get_zd_data(vars, ".TradeRecord.Request.MacId", mac);

		pub_log_debug("[%s][%d] the mac[%s]", __FILE__, __LINE__, mac);

		memset(filename, 0x00, sizeof(filename));
		sprintf(filename, "%s/tmp/.hide.txt", getenv("SWWORK"));
		fp = fopen(filename,"r+");
		if (fp == NULL)
		{
			strcpy(reply, "E016");
			pub_log_debug("file: [%s]open 失败! errno=[%s]:[%d] at [%s] [%d]\n", "xxx", errno, strerror(errno), __FILE__, __LINE__);
			goto ErrExit;
		}		

		while (!feof(fp))
		{
			memset(buf, 0x00, sizeof(buf));
			memset(tmp, 0x00, sizeof(tmp));
			fgets(buf, sizeof(buf) - 1, fp);

			if (strlen(buf) == 0 || buf[0] == '\0' || memcmp(buf, "continue", 8) == 0)
			{
				continue;
			}

			len = strlen(buf);
			if (buf[len - 1] == '\n')
			{
				buf[len - 1] = '\0';
			}		
			packdecrypt(buf, len);
			strcpy(tmp, buf);
			pub_log_debug("[%s][%d] tmp=[%s][%d] ", __FILE__, __LINE__, tmp, strlen(tmp));

			memset(file_key, 0x00, sizeof(file_key));
			ptmp = NULL;
			ptmp = strtok(buf, "|");
			if (ptmp == NULL || strlen(ptmp) == 0 || strncmp(ptmp, "continue", 8) == 0)
			{
				continue;
			}
			memcpy(file_key, ptmp, sizeof(file_key) - 1);

			memset(mactmp, 0x00, sizeof(mactmp));
			ptmp = NULL;
			ptmp = strtok(NULL, "|");
			if (ptmp == NULL || strlen(ptmp) == 0)
			{
				continue;
			}
			memcpy(mactmp, ptmp, sizeof(mactmp) -1);

			memset(file_user, 0x00, sizeof(file_user));
			ptmp = NULL;
			ptmp = strtok(NULL, "|");
			if (ptmp == NULL || strlen(ptmp) == 0)
			{
				continue;
			}
			memcpy(file_user, ptmp, sizeof(file_user) -1);

			memset(file_passwd, 0x00, sizeof(file_passwd));
			ptmp = NULL;
			ptmp = strtok(NULL, "|");
			if (ptmp == NULL || strlen(ptmp) == 0)
			{
				continue;
			}
			memcpy(file_passwd, ptmp, sizeof(file_passwd) -1);

			memset(role, 0x00, sizeof(role));
			ptmp = NULL;
			ptmp = strtok(NULL, "|");
			if (ptmp == NULL || strlen(ptmp) == 0)
			{
				continue;
			}
			memcpy(role, ptmp, sizeof(role) -1);

			len = strlen(role);
			if (role[len - 1] == '\n')
			{
				role[len - 1] = '\0';
			}

			pub_log_debug("[%s][%d]file_user=[%s] user=[%s]", __FILE__, __LINE__, file_user, user);
			if (strcmp(file_user, user) == 0)
			{
				pub_log_debug("[%s][%d]file_passwd=[%s] passwd=[%s]", __FILE__, __LINE__, file_passwd, passwd);
				if (strcmp(file_passwd, passwd) == 0)
				{
					len = strlen(tmp);
					pub_log_debug("[%s][%d] tmp=[%s][%d]", __FILE__, __LINE__, tmp, len);
					ret = fseek(fp, -len, SEEK_CUR);
					if (ret)
					{
						pub_log_error("[%s][%d] file修改失败!", __FILE__, __LINE__);
						fclose(fp);
						strcpy(reply, "E027");
						goto ErrExit;	
					}

					memset(buf, 0x00, sizeof(buf));
					sprintf(buf, "continue");
					fprintf(fp, "%s", buf);

					memset(key, 0x00, sizeof(key));
					ret = get_key(mactmp, user, newpasswd, role, key);
					if (ret != 0)
					{
						pub_log_error("[%s][%d] 密码修改失败!", __FILE__, __LINE__);
						strcpy(reply, "EO12");
						fclose(fp);
						goto ErrExit;	
					}	
					pub_log_debug("[%s][%d]newkey=[%s]", __FILE__, __LINE__, key);
					memset(buf, 0x00, sizeof(buf));
					sprintf(buf, "%s|%s|%s|%s|%s\n", key, mactmp, user, newpasswd, role);
					pub_log_debug("[%s][%d]buf=[%s]", __FILE__, __LINE__, buf);

					fseek(fp, 0, SEEK_END);
					len = strlen(buf);
					packencrypt(buf, len);
					fprintf(fp, "%s", buf);
					pub_mem_memzero(res_msg, sizeof(res_msg));
					sprintf(res_msg, "修改密码成功");
					pub_log_debug("[%s][%d]res_msg=[%s]", __FILE__, __LINE__, res_msg);
					fclose(fp);	
					goto OkExit;
				}
				else
				{
					pub_log_debug("[%s][%d]原密码不匹配", __FILE__, __LINE__);
					strcpy(reply, "E054");
					fclose(fp);
					goto ErrExit;
				}
			}
		}	

		if (feof(fp))
		{	
			pub_log_error("[%s][%d]用户不存在", __FILE__, __LINE__);
			strcpy(reply, "E053");
			goto ErrExit;
		}
	}	
OkExit:
	memset(buf,0x00,sizeof(buf));

	if (strlen(randnum) > 0)
	{
		ret = rannum(buf, res_msg, randnum);
		if (ret)
		{
			goto ErrExit;
		}

		memset(role, 0x00, sizeof(role));
		len = strlen(authid);
		strcpy(role, authid + len - 2);	
		pub_log_debug("[%s][%d] res_msg=[%s] role=[%s]", __FILE__, __LINE__, res_msg, role);
		loc_set_zd_data(vars, ".TradeRecord.Response.Role", role);
		loc_set_zd_data(vars, ".TradeRecord.Response.RandNum", buf);
	}
	pub_log_debug("[%s][%d] [%s]deal end![END][OK]", __FILE__, __LINE__, __FUNCTION__);
	strncpy(res_msg, "successful !!", sizeof(res_msg) - 1);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", "0000");

	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);
	return SW_OK;

ErrExit:
	pub_log_debug("[%s][%d] [%s]deal end![END][ERR]", __FILE__, __LINE__, __FUNCTION__);
	agt_error_info(reply, res_msg);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnCode", reply);
	loc_set_zd_data(vars, ".TradeRecord.Header.ReturnMessage", res_msg);

	return SW_ERROR;
}
