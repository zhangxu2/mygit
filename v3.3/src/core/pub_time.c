#include <sys/time.h>
#include <time.h>
#include "pub_time.h"
#include "pub_mem.h"

void pub_time_timezone_update(void)
{
	time_t	s;
	sw_tm_t	*t;
	char	buf[4];

	s = time(0);

	t = localtime(&s);

	strftime(buf, 4, "%H", t);
}

void pub_time_localtime(time_t s, sw_tm_t *tm)
{
#if (SW_HAVE_LOCALTIME_R)
	(void) localtime_r(&s, tm);

#else
	sw_tm_t  *t;

	t = localtime(&s);
	*tm = *t;

#endif

	tm->sw_tm_mon++;
	tm->sw_tm_year += 1900;
}

void pub_time_libc_localtime(time_t s, sw_tm_t *tm)
{
#if (SW_HAVE_LOCALTIME_R)
	(void) localtime_r(&s, tm);

#else
	sw_tm_t	*t;

	t = localtime(&s);
	*tm = *t;

#endif
}

void pub_time_gettmofstr(char *str, char *fmt, sw_tm_t *tm)
{
	sscanf(str, fmt,
		&(tm->tm_year), 
		&(tm->tm_mon),
		&(tm->tm_mday),
		&(tm->tm_hour),			
		&(tm->tm_min),
		&(tm->tm_sec));		
	/*tm->tm_year = tm->tm_year - 1900;*/		
	tm->tm_mon--;
}

/*errlog.c appGetTime logGetTime*/
sw_int_t pub_time_gettime(char *str, char *datefmt, char *datefmt1, char *usecfmt, sw_int_t num, sw_int_t len, char *timestr)
{
	sw_tm_t		tm;
	sw_timeb_t	timeb;
	
	memset(&tm, 0x00, sizeof(sw_tm_t));
	memset(&timeb, 0x00, sizeof(sw_timeb_t));

	if(str == NULL)
	{
		pub_time_ftime(&timeb);
	}
	else
	{
		pub_time_gettmofstr(str, datefmt, &tm); 	
		tm.tm_year = tm.tm_year - 1900;
		timeb.time = mktime(&tm);
	}

	timeb.time = timeb.time + num;

	(void)localtime_r(&(timeb.time), &tm);

	strftime(timestr, (size_t)(len + 1), datefmt1, &tm);
	if(str == NULL)
	{
		sprintf(timestr + len, usecfmt, timeb.millitm);
	}
	else
	{
		strcpy(timestr + len, str + len);
	}
	
	return SW_OK;	
}

/*errlog.c nGetTimeSpace*/
sw_int_t pub_time_minus(char *time1, char *time2)
{
	sw_tm_t	tm;
	sw_int_t ntime1, ntime2;

	memset(&tm, 0x00, sizeof(tm));

	pub_time_gettmofstr(time1, "%2d%2d%2d%2d%2d%2d", &tm);
	
	if(tm.tm_year < 50)
	{
		tm.tm_year += 100;
	}

	ntime1 = (sw_int_t)mktime(&tm);
	
	memset(&tm, 0x0, sizeof(tm));
	pub_time_gettmofstr(time2, "%2d%2d%2d%2d%2d%2d", &tm);	
	if(tm.tm_year < 50)
	{
		tm.tm_year += 100;
	}

	ntime2 = (sw_int_t)mktime(&tm);

	return (ntime2 - ntime1);
}

void pub_time_libc_gmtime(time_t s, sw_tm_t *tm)
{
#if (SW_HAVE_LOCALTIME_R)
	(void) gmtime_r(&s, tm);

#else
	sw_tm_t	*t;

	t = gmtime(&s);
	*tm = *t;

#endif
}

/*errlog.c appSleep*/
sw_int_t pub_time_timer(sw_int_t sec, sw_int_t usec)
{
	sw_int_t	ret;

	sw_timeval_t	time_out;

	memset(&time_out, 0x00, sizeof(sw_timeval_t));
	
	time_out.tv_sec = sec;
	time_out.tv_usec = usec;

	ret = select(0, NULL, NULL, NULL, &time_out);
	if(ret < 0)
	{
		return SW_ERROR;
	}
	else
	{
		return SW_OK;
	}
}

/*errlog.c iGetTimeBuf*/
sw_int_t pub_time_format(char *datefmt, char *timefmt, char *usecfmt, char *date, char *time, sw_int_t flag)
{
	sw_int_t	len;
	long long	utm;

	sw_timeval_t	tv;
	sw_tm_t		tm;

	memset(&tv, 0x00, sizeof(sw_timeval_t));
	memset(&tm, 0x00, sizeof(sw_tm_t));

	pub_time_gettimeofday(&tv);

	pub_time_localtime(tv.tv_sec, &tm);

	sprintf(time, timefmt, tm.tm_hour, tm.tm_min, tm.tm_sec);
	sprintf(date, datefmt, tm.tm_year, tm.tm_mon, tm.tm_mday);
	if(flag == 1)
	{
		utm = tv.tv_usec / 100;
		len = strlen(time);		
		if(usecfmt == NULL)
		{
			sprintf(time + len, ":%04lld", utm);
		}
		else
		{
			sprintf(time + len, usecfmt, utm);	
		}
	}
	
	return SW_OK;
}

sw_char_t* pub_time_app_get_time(char *input_time, sw_int32_t sec_num,char *ouput_time)
{
	struct tm time_value;
#ifdef WIN32
	struct _timeb time_b;
#else
	struct  timeb time_b;
#endif

	memset((char *)&time_value ,'\0',sizeof(struct tm));
#ifdef WIN32
	memset((char *)&time_b,'\0',sizeof(struct _timeb));
#else
	memset((char *)&time_b,'\0',sizeof(struct  timeb));
#endif

	if (input_time == NULL) {
#ifdef WIN32
		_ftime(&time_b);
#else
		 ftime(&time_b);
#endif
	} else {
		sscanf(input_time,"%4d%2d%2d%2d%2d%2d",&(time_value.tm_year),
			&(time_value.tm_mon),&(time_value.tm_mday),&(time_value.tm_hour),
			&(time_value.tm_min),&(time_value.tm_sec));

		time_value.tm_year = time_value.tm_year - 1900;
		time_value.tm_mon--;
		time_b.time = mktime(&time_value);
	}

	time_b.time = time_b.time + sec_num;

	#ifdef AIX
	memcpy(&time_value,localtime((time_t *)&(time_b.time)),sizeof(struct tm));
	#else
	memcpy(&time_value,localtime((long *)&(time_b.time)),sizeof(struct tm));
	#endif
	strftime(ouput_time,(size_t)(14 + 1),"%Y%m%d%H%M%S",&time_value);
	if (input_time == NULL) {
		sprintf(ouput_time + 14,"%.3d",time_b.millitm);
	} else {
		strcpy( ouput_time + 14,input_time + 14);
	}

	return(ouput_time);
}

sw_int_t pub_time_change_time(time_t *time, sw_char_t *out_put, sw_int32_t format_flag)
{
	struct tm *tm_t = NULL;
    
	if(NULL == time)
	{
		return -1;
	}
    else
	{
		tm_t=(struct tm*)localtime(time);
		if(tm_t!=NULL)
		{
			if(format_flag == 1)
			{
				sprintf(out_put,"%04d%02d%02d%02d:%02d:%02d",tm_t->tm_year+1900,tm_t->tm_mon+1,
				tm_t->tm_mday,tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
			}
			else if(format_flag == 0)
			{
				sprintf(out_put,"%02d:%02d:%02d",tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
			}
			else if(format_flag == 2)
			{
				sprintf(out_put,"%04d%02d%02d%02d%02d%02d",tm_t->tm_year+1900,tm_t->tm_mon+1,
				tm_t->tm_mday,tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
			}
			else if(format_flag == 3)
			{
				sprintf(out_put,"%04d%02d%02d",tm_t->tm_year+1900,tm_t->tm_mon+1,tm_t->tm_mday);
			}
			else if(format_flag == 4)
			{
				sprintf(out_put,"%04d年%02d月%02d日 %02d时%02d分%02d秒",tm_t->tm_year+1900,tm_t->tm_mon+1,
				tm_t->tm_mday,tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
			}
			else if(format_flag == 5)
			{
				sprintf(out_put,"%04d%02d%02d %02d:%02d:%02d",tm_t->tm_year+1900,tm_t->tm_mon+1,
				tm_t->tm_mday,tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
			}
		}
	}
	return 0;
}

sw_int64_t pub_time_get_current()
{
	sw_int64_t	time_value;
	struct timezone	tz;
	struct timeval	tv;
	
	pub_mem_memzero(&tz, sizeof(struct timezone));
	pub_mem_memzero(&tv, sizeof(struct timeval));

	gettimeofday(&tv, &tz);
	time_value = tv.tv_sec * 1000000LL + tv.tv_usec;

	return time_value;
}

sw_int64_t pub_time_get_space(sw_int64_t t1, sw_int64_t t2)
{
	sw_int64_t	space = 0;

	if (t1 > t2)
	{
		space = (t1 - t2) / 1000000;
	}
	else
	{
		space = (t2 - t1) / 1000000;
	}

	return space;
}

sw_int32_t pub_change_time2(sw_int64_t time_value, sw_char_t* time_buf, sw_int32_t flag)
{
	sw_int64_t	time_sec = 0;
	sw_int64_t	u_time = 0;
	sw_int32_t	len = 0;

	if(time_value < 0 || time_buf == NULL)
	{
		return 1;
	}
	
	time_sec =  time_value / 1000000;
	pub_time_change_time((time_t *)&time_sec, time_buf, flag);
	u_time = time_value % 1000000;
	u_time = u_time / 1000;
	len = strlen(time_buf);
	sprintf(time_buf + len, ".%03lld", u_time);

	return 0;
}

sw_int32_t pub_change_time(sw_int64_t time_value, sw_char_t* time_buf, sw_int32_t flag)
{
	sw_int64_t	time_sec = 0;

	if(time_value < 0 || time_buf == NULL)
	{
		return 1;
	}
	
	time_sec =  time_value / 1000000;
	pub_time_change_time((time_t *)&time_sec, time_buf, flag);

	return 0;
}

sw_int_t pub_time_getdate(char *date, int flag)
{
    struct tm	*pstm;
	struct timeb	tmb;
	
	if (date == NULL)
	{
		return SW_ERROR;
	}
	
	memset(&tmb, 0x0, sizeof(tmb));
	ftime(&tmb);
	pstm = localtime(&tmb.time);
	
	if (flag == 1) /*yyyy-mm-dd*/
	{
		strftime(date, 32, "%Y%m%d", pstm);
	}
	else if (flag == 2) /*HHMMSS*/
	{
		strftime(date, 32, "%H%M%S", pstm);
	}
	else if (flag == 3)
	{
		strftime(date, 32, "%Y%m%d%H%M%S", pstm);
	}
	
	return SW_OK;
}
