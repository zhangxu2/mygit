#include "pub_errno.h"
#include "pub_string.h"


sw_str_t  *pub_sys_errlist;
static sw_str_t   pub_unknown_error = pub_string("Unknown error");

u_char *
pub_err_strerror(sw_err_t err, u_char *errstr, size_t size)
{
	sw_str_t  *msg;

	msg = ((sw_uint_t) err < SW_SYS_NERR) ? &pub_sys_errlist[err]:
                                              &pub_unknown_error;
	size = pub_math_min(size, msg->len);

	/*return ngx_cpymem(errstr, msg->data, size);*/
	return memcpy(errstr, msg->data, size);
}

sw_uint_t
pub_err_init(void)
{
	char       *msg;
	char	*p;
	size_t      len;
	sw_err_t   err;

	/*
     	* pub_err_strerror() is not ready to work at this stage, therefore,
     	* malloc() is used and possible errors are logged using strerror().
     	*/

	len = SW_SYS_NERR * sizeof(sw_str_t);

	pub_sys_errlist = malloc(len);
	if (pub_sys_errlist == NULL) 
	{
        	goto failed;
	}

	for (err = 0; err < SW_SYS_NERR; err++) 
	{
        	msg = strerror(err);
        	len = strlen(msg);

        	p = malloc(len);
        	if (p == NULL) 
		{
            		goto failed;
        	}

        	memcpy(p, msg, len);
        	pub_sys_errlist[err].len = len;
        	pub_sys_errlist[err].data = p;
	}

	return SW_OK;

failed:

    	err = errno;

    	return SW_ERROR;
}
