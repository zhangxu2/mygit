#include <stdio.h>
#include "pub_hash.h"

#define Max_Num		7
#define Max_Size	1024
#define Bucket_Size	64
#define Max_Url_Len 15
#define Max_Ip_Len 15 
#define NGX_HASH_ELT_SIZE(name)			\
    (sizeof(void *) + sw_align((name)->key.len + 2, sizeof(void *)))

static sw_str_t urls[Max_Num] = {  
    pub_string("www.baidu.com"),  
    pub_string("www.sina.com.cn"), 
    pub_string("www.google.com"),
    pub_string("www.qq.com"),
    pub_string("www.163.com"),
    pub_string("www.sohu.com"),
    pub_string("www.abo321.org")
};
  
static char* values[Max_Num] = {  
    "220.181.111.147",  
    "58.63.236.35",  
    "74.125.71.105",  
    "60.28.14.190",  
    "123.103.14.237",  
    "219.234.82.50",  
    "117.40.196.26"  
};

void dump_hash(sw_hash_t *hash, sw_array_t *array)  
{  
	int loop;  
	char prefix[] = "          ";  
	u_short test[Max_Num] = {0};  
	sw_uint_t key;  
	sw_hash_key_t* elts;  
	int nelts;  
  
	if (hash == NULL)
	{
		return;
	}
  
	printf("hash = 0x%x: **buckets = 0x%x, size = %d\n", hash, hash->buckets, hash->size);  
  
	for (loop = 0; loop < hash->size; loop++)  
	{  
		sw_hash_elt_t *elt = hash->buckets[loop];  
		printf("  0x%x: buckets[%d] = 0x%x\n", &(hash->buckets[loop]), loop, elt);  
	}  
	printf("\n");  
  
	elts = (sw_hash_key_t*)array->elts;  
	nelts = array->nelts;  
	for (loop = 0; loop < nelts; loop++)  
	{  
		char url[Max_Url_Len + 1] = {0};  
	  
	        key = elts[loop].key_hash % hash->size;  
		sw_hash_elt_t *elt = (sw_hash_elt_t *) ((u_char *) hash->buckets[key] + test[key]);  
  
		pub_str_strlow(url, elt->name, elt->len);  
		printf("  buckets %d: 0x%x: {value = \"%s\"%.*s, len = %d, name = \"%s\"%.*s}\n",   
			key, elt, (char*)elt->value, Max_Ip_Len - strlen((char*)elt->value), prefix,   
			elt->len, url, Max_Url_Len - elt->len, prefix); 
  
		test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&elts[loop]));  
	}  
}  


sw_array_t* add_urls_to_array(sw_pool_t *pool)  
{  
	printf("****************add urls to array start ****************\n");
	int loop;  
	sw_array_t *a = pub_array_create(pool, Max_Num, sizeof(sw_hash_key_t));  
  
	for (loop = 0; loop < Max_Num; loop++)  
	{  
		sw_hash_key_t *hashkey = (sw_hash_key_t*)pub_array_push(a);  
		hashkey->key = urls[loop];  
		printf("key = [%s]\n", hashkey->key.data);
		hashkey->key_hash = pub_hash_key_lc(urls[loop].data, urls[loop].len);
		printf("key_hash = [%ld]\n", hashkey->key_hash);  
		hashkey->value = (void*)values[loop];
		printf("value = [%s]\n\n", hashkey->value);
	}  
  
	printf("****************add urls to array end ****************\n\n\n");
	return a;      
}  

void find_test(sw_hash_t *hash, sw_str_t addr[], int num)  
{  
    sw_uint_t key;  
    int loop;  
    char prefix[] = "          ";  
  
    for (loop = 0; loop < num; loop++)  
    {  
        key = pub_hash_key_lc(addr[loop].data, addr[loop].len);  
        void *value = pub_hash_find(hash, key, addr[loop].data, addr[loop].len);  
        if (value)  
        {  
            printf("(url = \"%s\"%.*s, key = %-10ld) found, (ip = \"%s\")\n",   
                addr[loop].data, Max_Url_Len - addr[loop].len, prefix, key, (char*)value);  
        }  
        else  
        {  
            printf("(url = \"%s\"%.*s, key = %-10d) not found!\n",   
                addr[loop].data, Max_Url_Len - addr[loop].len, prefix, key);  
        }  
    }  
}  


int main()
{
	sw_int_t result;  
	sw_hash_init_t hinit; 
	sw_pool_t *pool = NULL;
	sw_array_t *array = NULL;
	sw_hash_t *hash;

	pool = pub_pool_create(1024);  
	array = add_urls_to_array(pool);
	
	sw_cacheline_size = 32;
	hinit.hash = NULL;
	/*hinit.key = &pub_hash_key_lc;*/
	hinit.max_size = Max_Size;  
	hinit.bucket_size = Bucket_Size;  
	/*hinit.name = "my_hash_sample";*/  
	hinit.pool = pool;
	hinit.temp_pool = NULL;
 
	result = pub_hash_create(&hinit, (sw_hash_key_t*)array->elts, array->nelts);  
	if (result != SW_OK)
	{
		return -1;  
	}
	
	hash = hinit.hash;
	if (hash == NULL)
	{
		printf("create hash error!\n");
	}
	printf("create hash success!\n\n");
	printf("The hash test result as fllows:\n");
	dump_hash(hash, array);
	
	printf("\nfind test:\n");
	find_test(hash, urls, Max_Num);
	
	pub_array_destroy(array);
	pub_pool_destroy(pool);
	
	return 0; 
}
