#include "pub_log.h"
#include "pub_type.h"
#include "pub_vars.h"
#include "pub_buf.h"

int zip0a09(char *buf)
{
	int	len = 0;
	char	*str = buf;
	char	*ptr = buf;
	
	while (*str != '\0')
	{
		if (*str != 0x0a && *str != 0x09)
		{
			*ptr++ = *str;
			len++;
		}
		str++;
	}
	*ptr = '\0';
	
	return len;
}

int zip_0a09(sw_loc_vars_t *vars, sw_buf_t *buf, int flag)
{
	if (flag == SW_ENC)
	{
		char	tmp[32];
	
		memset(tmp, 0x0, sizeof(tmp));
		pub_log_info("[%s][%d] zip begin...", __FILE__, __LINE__);
		buf->len = zip0a09(buf->data);
		sprintf(tmp, "%07d", buf->len - 61);
		memcpy(buf + 54, tmp, 7);
		return 0;
	}
	
	return 0;
}

