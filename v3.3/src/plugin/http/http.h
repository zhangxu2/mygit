#ifndef HTTP_H
#define HTTP_H

#if defined(SW_USE_OPENSSL)
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct _ssl_cycle
{
	SSL_CTX *ctx;
	char *cafile;
	char *keyfile;
}SSLCycle;
SSLCycle        sslcycle;

typedef struct _ssl_count
{
	SSL *ssl;
	sw_int_t sock;
	sw_int_t used;
}SSLCount;
SSLCount        *sslcount;
#endif

int	g_is_ssl;

sw_int_t http_clear_ssl(sw_int_t index);
sw_int_t http_close_fd(sw_lsn_cycle_t *cycle, int index);
sw_int_t http_destroy_ssl(sw_lsn_cycle_t *cycle);
sw_int_t http_accept_ssl(sw_lsn_cycle_t *cycle, sw_fd_t acceptfd, int index);
sw_int_t http_connect(sw_lsn_cycle_t *cycle);
sw_int_t http_getsys_date(time_t *time_now, sw_char_t *date);
sw_int_t http_res_err(sw_lsn_cycle_t *cycle, int index, int state);
sw_int_t http_res_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, char *head, sw_int_t *headlen, sw_int_t content_len);
sw_int_t http_req_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, char *head, sw_int_t *headlen, sw_int_t content_len);
sw_int_t http_getsys_date(time_t *time_now, sw_char_t *date);
sw_int_t http_send(sw_lsn_cycle_t *cycle, sw_char_t *buf, sw_int_t length, int index);
sw_int_t http_recv(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, int index);
int http_add_head(sw_lsn_cycle_t *cycle, sw_loc_vars_t *locvar, sw_buf_t *locbuf, int flag);
int http_unpack_head(sw_loc_vars_t *vars, sw_buf_t *pkgbuf);

#endif
