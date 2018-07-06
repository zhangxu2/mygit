#ifndef __PUB_CODE_H__
#define __PUB_CODE_H__

#include "pub_type.h"

sw_int_t pub_code_base64_enc(unsigned char *src_code, unsigned char *dst_code, sw_int_t src_len);
sw_int_t pub_code_base64_dec(unsigned char *src_code, unsigned char *dst_code, sw_int_t *dst_len);
sw_int_t pub_code_convert(char *from, char *to, char *src, size_t *src_len, char *dst, size_t *len);
sw_int_t pub_code_u2g(char *src, size_t *src_len, char *dst, size_t *dst_len);
sw_int_t pub_code_g2u(char *src, size_t *src_len, char *dst, size_t *dst_len);
sw_int_t pub_code_asctobcd(char *src, char *dst, sw_int_t src_len);
sw_int_t pub_code_asctobcd_right(char *src, char *dst, sw_int_t src_len);
sw_int_t pub_code_asctobcd_left(char *src, char *dst, sw_int_t src_len);

sw_int_t pub_code_bcdtoasc(char *src, char *dst, sw_int_t src_len);
sw_int_t pub_code_Compress( unsigned char *in, unsigned char *out, unsigned int insize );
sw_int_t pub_code_CompressFast( unsigned char *in, unsigned char *out,unsigned int insize, unsigned int *work );
sw_int_t pub_code_Uncompress( unsigned char *in, unsigned char *out,unsigned int insize ,unsigned int *outsize);

sw_int_t pub_code_numtohost(int num,char *buf,int len);
sw_int_t pub_code_numtonet(int num, char *buf, int len);
sw_int_t pub_code_nettonum(char *buf,int len);
sw_int_t pub_code_hosttonum(char *buf, int len);

int pub_code_hex2int(u_char *ptr, int len);
int pub_code_int2hex(u_char *ptr, int value, int len);





#endif

