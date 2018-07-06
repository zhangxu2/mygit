#include "seqs.h"
#include "pub_mem.h"
#include "pub_log.h"
#include "run.h"
#include "pub_shm.h"

int main(int argc, char* argv[])
{
	sw_char_t	shm_trace[1024];
	sw_int_t	result = SW_OK;
	sw_int64_t	trace_no = -1;
	sw_int32_t	i = 0, j = 0, k = 0;
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
	}

	syscfg.seq_max = 64;
	*/
	/*set shm_run key
	size = seqs_get_size(&syscfg);

	shm_seqs = malloc(size);
	if (shm_seqs == NULL)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = seqs_init(shm_seqs, &syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}*/

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__FILE__);
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, shm_trace_no size(%d)", __FILE__, __LINE__, size);

#define LOOP_BEGIN for (i = 0; i < 100000; i++)\
		   {\
		   for (j = 0; j < 100000; j++)\
		   {\
		   for (k = 0; k < 100000; k++)\
		   {

#define LOOP_END   }\
		   }\
		   }
	
	/*test new trace no*/
	trace_no = seqs_new_trace_no();
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, stat_init.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, get trace_no[%lld]", __FILE__, __LINE__, trace_no);

	/*test get trace no*/
	trace_no = seqs_get_trace_no();
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, stat_get_trace_no.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, get trace_no[%lld]", __FILE__, __LINE__, trace_no);
	
	
	/*test add business trace no generator*/
	sw_char_t	trc1[16] = {"cnaps1"};
	result = seqs_add_business_trace(trc1, 1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_add_business_trace error, name[%s]."
				, __FILE__, __LINE__, trc1);
		return SW_ERROR;	
	}

	sw_char_t	trc2[16] = {"cnaps2"};
	result = seqs_add_business_trace(trc2, 2);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_add_business_trace, name[%s]."
				, __FILE__, __LINE__, trc2);
		return SW_ERROR;	
	}

	result = seqs_add_business_trace(trc2, 2);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_add_business_trace, name[%s]."
				, __FILE__, __LINE__, trc2);
		return SW_ERROR;	
	}

	result = seqs_add_business_trace(trc2, 2);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_add_business_trace, name[%s]."
				, __FILE__, __LINE__, trc2);
		return SW_ERROR;	
	}

	/*LOOP_BEGIN*/

	/*test get new business trace no*/
	trace_no = seqs_new_business_no(trc1, 1);
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, seqs_new_business_no error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_new_business_no trace_no[%lld]"
			, __FILE__, __LINE__, trace_no);

	trace_no = seqs_new_business_no(trc2, 2);
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, seqs_new_business_no error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_new_business_no trace_no[%lld]"
			, __FILE__, __LINE__, trace_no);

	trace_no = seqs_new_business_no(trc2, 2);
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, seqs_new_business_no error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_new_business_no trace_no[%lld]"
			, __FILE__, __LINE__, trace_no);

	/*test save trace stat*/
	result = seqs_save(1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_save error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_save"
			, __FILE__, __LINE__);

	/*test recover trace stat*/
	result = seqs_recover();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_recover error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, seqs_recover"
			, __FILE__, __LINE__);

	trace_no = seqs_new_business_no(trc1, 1);
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, seqs_new_business_no error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_new_business_no trace_no[%lld]"
			, __FILE__, __LINE__, trace_no);

	trace_no = seqs_new_business_no(trc2, 2);
	if (trace_no == SW_ERROR)
	{
		pub_log_error("%s, %d, seqs_new_business_no error, trace no %lld."
				, __FILE__, __LINE__, trace_no);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_new_business_no trace_no[%lld]"
			, __FILE__, __LINE__, trace_no);
			
	/*test change date*/
	result = seqs_change_date("20130606");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_change_date error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}

	sw_char_t	sysdate[16];
	pub_mem_memzero(sysdate, sizeof(sysdate));
	result = seqs_get_sysdate(sysdate);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, seqs_get_sysdate error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, seqs_get_sysdate, sysdate[%s]"
			, __FILE__, __LINE__, sysdate);
	/*LOOP_END*/

	return SW_OK;
}

