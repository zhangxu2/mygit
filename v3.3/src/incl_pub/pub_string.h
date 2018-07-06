#ifndef __PUB_STRING_H__
#define __PUB_STRING_H__

#include "pub_type.h"
#include "pub_math.h"

typedef struct 
{
	size_t len;
	sw_char_t *data;
} sw_str_t;

typedef struct 
{
	sw_str_t key;
	sw_str_t value;
} sw_keyval_t;

typedef struct 
{
	unsigned len:28;

	unsigned valid:1;
	unsigned no_cacheable:1;
	unsigned not_found:1;
	unsigned escape:1;

	sw_char_t   *data;
} sw_variable_value_t;

#define pub_string(str) { sizeof(str) - 1,(sw_char_t *)str}
#define pub_null_string { 0, NULL }

#define pub_str_set(str,text) (str)->len = sizeof(text) - 1; (str)->data = (sw_char_t *)text
#define pub_str_null(str) (str)->len = 0;\
	(str)->data = NULL

#define pub_str_tolower(c) (sw_char_t)(((c >= 'A' && c <= 'Z')) ? (c | 0x20) : c)
#define pub_str_toupper(c) (sw_char_t)(((c >= 'a' && c <= 'z')) ? (c & ~0x20) : c)

#define pub_str_isempty(str) ((NULL==str) || ('\0'==str[0]))

int pub_str_strlow(sw_char_t *dst, const sw_char_t *src, size_t n);

#define pub_str_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)

#define pub_str_strcmp(s1, s2) strcmp((const char *) s1,(const char*) s2)

#define pub_str_strstr(s1, s2) strstr((const char *) s1,(const char *) s2)
#define pub_str_strlen(s)      strlen((const char *) s)

#define pub_str_strchr(s1, c)  strchr((const char *) s1, (int)c)


sw_int_t pub_str_ziphlspace(sw_char_t *str);
int pub_str_zipspace(sw_char_t *src);
int pub_str_zip0a09(char *buf, int begin);
int pub_str_cpyext(sw_char_t *dst,sw_char_t *src);
sw_char_t *pub_str_cpystrn(sw_char_t *dst, const sw_char_t *src, size_t n);

int pub_str_strcasecmp(sw_char_t *s1, sw_char_t *s2);
int pub_str_strncasecmp(sw_char_t *s1, sw_char_t *s2, size_t n);

sw_char_t *pub_str_strnstr(sw_char_t *s1, char *s2, size_t n);

sw_char_t *pub_str_strstrn(sw_char_t *s1, char *s2, size_t n);
sw_char_t *pub_str_strcasestrn(sw_char_t *s1, char *s2, size_t n);
sw_char_t *pub_str_strlcasestrn(sw_char_t *s1, sw_char_t *last, sw_char_t *s2, size_t n);

int pub_str_rstrncmp(sw_char_t *s1, sw_char_t *s2, size_t n);
int pub_str_rstrncasecmp(sw_char_t *s1, sw_char_t *s2, size_t n);
int pub_str_memn2cmp(sw_char_t *s1, sw_char_t *s2, size_t n1, size_t n2);
int pub_str_atoi(sw_char_t *src, size_t n);
ssize_t pub_str_atosz(sw_char_t *src, size_t n);
off_t pub_str_atoof(sw_char_t *src, size_t n);
time_t pub_str_atotm(sw_char_t *src, size_t n);
int pub_str_hextoi(sw_char_t *src, size_t n);
sw_char_t *pub_str_hex_dump(sw_char_t *dst, sw_char_t *src, size_t len);

int pub_str_ltrim(char *src);
int pub_str_rtrim(char *src);
int pub_str_trim(char *src);

int pub_str_replace(char *str1, const char *str2, const char *str3);
int pub_str_replaceall(char *str1, const char *str2, const char *str3);
int pub_str_substring(char *dst, char *src, size_t start, size_t end);
char pub_str_charat(char *src, size_t index);
int pub_str_indexof(char *str1, char *str2);
int pub_str_lastindexof(char *str1, char *str2);

sw_char_t *pub_str_sprintf(sw_char_t *buf, const char *fmt, ...);
sw_char_t *pub_str_snprintf(sw_char_t *buf, size_t max, const char *fmt, ...);
sw_char_t *pub_str_slprintf(sw_char_t *buf, sw_char_t *last, const char *fmt, ...);
sw_char_t *pub_str_vslprintf(sw_char_t *buf, sw_char_t *last, const char *fmt, va_list args);
char *pub_str_msstrtok(char *instr, char *outstr, char *delimiter);
int pub_str_strcpy_ext(char *s1,char *s2);
sw_int_t pub_str_checkincl(sw_char_t *include,sw_char_t *str);
bool str_isdigit(const char *str);
#define strlen_iszero(str) ('\0' == str[0])
#define str_isempty(str) pub_str_isempty(str)

int pub_str_strrncmp(char *s1, char *s2, size_t n);
int pub_str_include(char *var, char *str);

#endif
