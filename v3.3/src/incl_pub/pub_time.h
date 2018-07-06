#ifndef __PUB_TIME_H__
#define __PUB_TIME_H__

#include "pub_type.h"

#define DATE_MAX_LEN    (16)    /* 日期最大长度 */
#define TIME_MAX_LEN    (32)    /* 时间最大长度 */
#define DTIME_MAX_LEN   (32)    /* 日期时间最大长度 */

typedef struct tm	sw_tm_t;
typedef struct timeval	sw_timeval_t;

#ifdef WIN32
typedef struct _timeb	sw_timeb_t;
#define pub_time_ftime	_ftime
#else
typedef struct timeb	sw_timeb_t;
#define pub_time_ftime	ftime
#endif

#define sw_tm_sec	tm_sec
#define sw_tm_min	tm_min
#define sw_tm_hour	tm_hour
#define sw_tm_mday	tm_mday
#define sw_tm_mon	tm_mon
#define sw_tm_year	tm_year
#define sw_tm_wday	tm_wday
#define sw_tm_isdst	tm_isdst

#define sw_tm_sec_t	int
#define sw_tm_min_t	int
#define sw_tm_hour_t	int
#define sw_tm_mday_t	int
#define sw_tm_mon_t	int
#define sw_tm_year_t	int
#define sw_tm_wday_t	int

#define pub_time_timezone(isdst) (- (isdst ? timezone + 3600 : timezone) / 60)

SW_PUBLIC void pub_time_timezone_update(void);
SW_PUBLIC void pub_time_localtime(time_t s, sw_tm_t *tm);
SW_PUBLIC void pub_time_libc_localtime(time_t s, sw_tm_t *tm);
SW_PUBLIC void pub_time_libc_gmtime(time_t s, sw_tm_t *tm);
SW_PUBLIC void pub_time_gettmofstr(char *str, char *fmt, sw_tm_t *tm);
SW_PUBLIC sw_int_t pub_time_timer(sw_int_t sec, sw_int_t usec);
SW_PUBLIC sw_int_t pub_time_gettime(char *str, char *datefmt, char *datefmt1, char *usecfmt
		, sw_int_t num, sw_int_t len, char *timestr);
SW_PUBLIC sw_int_t pub_time_format(char *datefmt, char *timefmt, char *usecfmt, char *date
		, char *time, sw_int_t flag);
SW_PUBLIC sw_int_t pub_time_minus(char *time1, char *time2);
SW_PUBLIC sw_char_t* pub_time_app_get_time(char *input_time, sw_int32_t sec_num,char *ouput_time);
SW_PUBLIC sw_int_t pub_time_change_time(time_t *time, sw_char_t *out_put, sw_int32_t format_flag);
SW_PUBLIC sw_int64_t pub_time_get_current();
SW_PUBLIC sw_int32_t pub_change_time(sw_int64_t time_value, sw_char_t* time_buf, sw_int32_t flag);
SW_PUBLIC sw_int32_t pub_change_time2(sw_int64_t time_value, sw_char_t* time_buf, sw_int32_t flag);
SW_PUBLIC sw_int_t pub_time_getdate(char *date, int flag);
SW_PUBLIC sw_int64_t pub_time_get_space(sw_int64_t t1, sw_int64_t t2);

#define pub_time_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define pub_time_msleep(ms)        (void) usleep(ms * 1000)
#define pub_time_sleep(s)          (void) sleep(s)

#endif

