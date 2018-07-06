#include "job.h"
#include "pub_mem.h"
#include "pub_log.h"
#include "pub_type.h"
#include "sem_lock.h"
#include "run.h"
#include "pub_shm.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_job_t	*shm_job = NULL;
	sw_int_t	i = 0;
	sw_int_t	count = 0;
	sw_int32_t	size = 0;
	sw_syscfg_t	syscfg;
	sw_int32_t	index = 0;
	sw_int32_t	tmp = 0;
	sw_sem_lock_t	*sem_lock = NULL;
	key_t		key = 0x33;
	sw_int32_t	shmid = -1;
	sw_char_t	*addr = NULL;
	sw_seqs_head_t	seqs_head;

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
	
	pub_log_info("%s, %d, after sem_loc_lock_creat", __FILE__, __LINE__);
	*/

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	if (seqs_get(&seqs_head))
	{
		pub_log_error("[%s][%d]take system date error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("[%s][%d] platform date=[%s] vfs_date[%s] busi_trc_cnt[%d] date_lock_id[%d] lock_flag[%d] max_busi_seq[%d] status[%d] sw_trn_base[%lld] sw_trn_prefix[%lld] trc_lock_id[%d]"
			, __FILE__, __LINE__, seqs_head.sys_date, seqs_head.vfs_date, seqs_head.busi_trc_cnt
			, seqs_head.date_lock_id, seqs_head.lock_flag, seqs_head.max_busi_seq, seqs_head.status
			, seqs_head.sw_trn_base, seqs_head.sw_trn_prefix, seqs_head.trc_lock_id);
			

	/*set shm_run key
	pub_mem_memzero(&syscfg, sizeof(syscfg));
	syscfg.job_max = 64;
	
	size = job_get_size(&syscfg);
	if (size == -1)
	{
		pub_log_error("%s, %d, job_get_size error, size[%d]."
				,__FILE__,__LINE__,size);
		return SW_ERROR;
	}*/

	shm_job = run_get_job();
	if (shm_job == NULL)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	/*result = job_init(shm_job, &syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	
	result = job_load_cfg("job.xml");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_load_cfg error."
				,__FILE__,__LINE__);
		return SW_ERROR;
	}

	count = shm_job->head.cur_cnt;
	for (i = 0; i < count; i++)
	{
		pub_log_info("%s, %d, job_name[%s] job_pid[%d] job_status[%c] last_run_time[%ld] manual[%d] no[%s] run_time[%s] run_type[%d] start_time[%d] time_out[%d] run_type[%d] week[%s] date[%s]"
				, __FILE__, __LINE__
				, shm_job->job_item[i].job_name, shm_job->job_item[i].job_pid
				, shm_job->job_item[i].job_status, shm_job->job_item[i].last_run_time
				, shm_job->job_item[i].manual
				, shm_job->job_item[i].no, shm_job->job_item[i].time
				, shm_job->job_item[i].exec_type, shm_job->job_item[i].start_time
				, shm_job->job_item[i].time_out, shm_job->job_item[i].run_type
				, shm_job->job_item[i].week, shm_job->job_item[i].date);
	}

	result = job_loc_init(shm_job, &syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_loc_init error.",__FILE__,__LINE__);
		return SW_ERROR;
	}
	*/


	result = job_loc_load_cfg(shm_job, "job.xml");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, job_loc_load_cfg error."
				,__FILE__,__LINE__);
		return SW_ERROR;
	}

	count = shm_job->head.cur_cnt;
	for (i = 0; i < count; i++)
	{
		pub_log_info("%s, %d, job_name[%s] job_pid[%d] job_status[%c] last_run_time[%ld] manual[%d] no[%s] run_time[%s] run_type[%d] start_time[%d] time_out[%d] run_type[%d] week[%s] date[%s]"
				, __FILE__, __LINE__
				, shm_job->job_item[i].job_name, shm_job->job_item[i].job_pid
				, shm_job->job_item[i].job_status, shm_job->job_item[i].last_run_time
				, shm_job->job_item[i].manual
				, shm_job->job_item[i].no, shm_job->job_item[i].time
				, shm_job->job_item[i].exec_type, shm_job->job_item[i].start_time
				, shm_job->job_item[i].time_out, shm_job->job_item[i].run_type
				, shm_job->job_item[i].week, shm_job->job_item[i].date);
	}	
	
	return SW_OK;
}

