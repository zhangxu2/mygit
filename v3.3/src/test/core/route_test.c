#include "trace.h"
#include "pub_mem.h"
#include "pub_log.h"
#include "sem_lock.h"
#include "pub_time.h"
#include "run.h"
#include "pub_cfg.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_char_t	*shm_trace = NULL;
	sw_ejf_info_t	ejf;
	sw_int32_t	i = 0;
	sw_int32_t	count = 30;
	sw_int32_t	mtype = 0;
	sw_int32_t	trace_no = 1;
	sw_char_t	buf[40000];
	sw_int32_t	size = 0;
	sw_syscfg_t	syscfg;
	sw_sem_lock_t	*sem_lock = NULL;
	key_t		key = 0x33;
	sw_int32_t	shmid = -1;
	sw_char_t	*addr = NULL;
	sw_seqs_t	*shm_seqs = NULL;

	/*shmid = pub_shm_create(key, sizeof(sw_sem_lock_t));
	if (shmid == SW_ERROR)
	{
		pub_log_error("%s, %d, pub_shm_create error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, shmid[%d]",__FILE__,__LINE__,shmid);
	addr = pub_shm_at(shmid);
	if ((void*)-1 == addr)
	{
		pub_log_error("%s, %d, pub_shm_at error, shmid[%d].",__FILE__,__LINE__,shmid);
		return SW_ERROR;		
	}
	
	sem_lock = (sw_sem_lock_t*)addr;
	pub_log_info("%s, %d, shmid[%d] addr[%p] sem_id[%d]"
			, __FILE__, __LINE__, shmid, addr, sem_lock->sem_id);

	sem_lock_set_addr(sem_lock);

	pub_mem_memzero(&syscfg, sizeof(sw_syscfg_t));
	syscfg.semsize = 128;

	pub_log_info("%s, %d, before sem_loc_lock_creat", __FILE__, __LINE__);

	result = sem_lock_link(sem_lock);
	if (result != SW_OK)
	{
		pub_log_info("%s, %d, sem_lock_link error[%d][%s].", __FILE__, __LINE__,errno, strerror(errno));

		result = sem_lock_creat(sem_lock, &syscfg);
		if (result <= 0)
		{
			pub_log_error("%s, %d, sem_lock_creat error.", __FILE__, __LINE__);
			return SW_ERROR;
		}
		pub_log_info("%s, %d, create semid[%d]",__FILE__,__LINE__,sem_lock->sem_id);
	}*/

	/*set shm_run key
	pub_mem_memzero(&syscfg, sizeof(syscfg));
	syscfg.session_max = 1000;

	size = trace_get_size(&syscfg);
	shm_trace = malloc(size);
	if (shm_trace == NULL)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = trace_init(shm_trace, &syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, trace_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__FILE__);
		return SW_ERROR;
	}
	
	mtype = 1;

	pub_mem_memzero(&ejf, sizeof(ejf));
	ejf.trace_no = trace_no;
	ejf.route.module_type = 10;
	ejf.route.module_num = 1;
	ejf.route.status = 1;
	pub_time_getdate(ejf.start_date, 1);
	
	sprintf(ejf.route.node, "first_node");
	
	result = trace_create_info(mtype, &ejf);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, trace_create_info error."
				,__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	for (i = 0; i < count; i++)
	{
		pub_mem_memzero(&ejf, sizeof(ejf));
		ejf.busi_no = i + 1;
		ejf.next_trno = i + 2;
		ejf.seqs = i;
		sprintf(ejf.start_date, "2013%0*d", 4, i);
		ejf.trace_no = trace_no;
		sprintf(ejf.tx_code, "%0*d", 4, i);
		sprintf(ejf.tx_respcd, "%0*d", 4, i);
		ejf.route.end_time = i + 6;
		ejf.route.error_code = i + 7;
		ejf.route.module_num = i + 8;
		ejf.route.module_type = i + 9;
		sprintf(ejf.route.node, "node%d", i);
		ejf.route.start_time = pub_time_get_current();
		ejf.route.end_time = ejf.route.start_time + 1;
		ejf.route.status = i + 11;
		ejf.route.tx_type = i + 12;

		if (i%2 == 0)
		{
			result = trace_insert_ejf(mtype, &ejf, TRACE_IN);
		}
		else
		{
			result = trace_insert_ejf(mtype, &ejf, TRACE_OUT);
		}

		if (result != SW_OK)
		{
			pub_log_error("%s, %d, trace_insert_ejf %d error."
					,__FILE__,__LINE__,i);
			return SW_ERROR;
		}

	}

	result = trace_insert_svr(mtype, "test_server", "test_service", trace_no);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, trace_insert_svr error.", __FILE__, __LINE__);
		return SW_OK;
	}
	
	result = trace_insert_ejf(mtype, &ejf, TRACE_OVER);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, trace_insert_ejf error."
				,__FILE__,__LINE__);
		return SW_ERROR;
	}	

	pub_mem_memzero(buf, sizeof(buf));
	result = trace_show_info(trace_no, buf);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, trace_show_info", __FILE__, __LINE__);
		return SW_OK;
	}

	return SW_OK;
}

