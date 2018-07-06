/*********************************************************************
 *** version : v3.0
 *** author  : wangkun
 *** create  : 2013-06-04
 *** module  : unit test
 *** name    : pub_mtype_test.c
 *** function: pub_mtype.c unit test
 *** notice  :
 *** modified:
 ***   author:
 ***   date  :
 ***  content:
 ********************************************************************/

#include "mtype.h"
#include "sem_lock.h"
#include "pub_mem.h"
#include "run.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_int32_t	sem_key = 0x33;
	sw_int32_t	sem_num = 200;
	sw_char_t	shm_addr[100000];
	sw_char_t	lock_addr[256];
	sw_int32_t	i = 0;
	sw_int32_t	mtype = 0;
	sw_int32_t	size = 0;
	sw_int32_t	count = 0;
	sw_syscfg_t	syscfg;
	sw_int32_t	index = 0;
	sw_int32_t	tmp = 0;
	sw_sem_lock_t	*sem_lock = NULL;
	key_t		key = 0x33;
	sw_int32_t	shmid = -1;
	sw_char_t	*addr = NULL;
	sw_char_t	*shm_mtype = NULL;

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
	*/
	/*set shm_run key
	syscfg.session_max = 1000;
	size = mtype_get_size(&syscfg);
	if (size == -1)
	{
		pub_log_error("%s, %d, mtype_get_size error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	shm_mtype = malloc(size);
	if (shm_mtype == NULL)
	{
		pub_log_error("%s, %d, malloc error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	result = mtype_init(shm_mtype, &syscfg);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_init error.", __FILE__, __LINE__);
		return SW_ERROR;
	}*/

	result = run_link();
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, run_link error.",__FILE__,__LINE__);
		return SW_ERROR;
	}

	count = mtype_get_max();
	pub_log_info("%s, %d, mtype shm max_count[%d]"
			, __FILE__, __LINE__, count);

	for (i = 0; i < count - 1; i++)
	{
		mtype = mtype_new();
		if (mtype == SW_ERROR)
		{
			pub_log_error("%s, %d, mtype_new error, i[%d] mtype[%d] count[%d]."
				, __FILE__, __LINE__, i, mtype, count);
			return SW_ERROR;
		}
	}

	/*test get max_type, cur_cnt*/
	pub_log_info("%s, %d, max_mtype[%d] cur_cnt[%d]"
			, __FILE__, __LINE__, mtype_get_max(), mtype_cur_cnt());
	
	/*test up-bound*/
	mtype = mtype_new();
	if (mtype != SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_new error, mtype[%d]."
			, __FILE__, __LINE__, mtype);
		return SW_ERROR;
	}

	for (i = 0; i < count - 2; i++)
	{
		result = mtype_delete(i + 1, 0);
		if (result == SW_ERROR)
		{
			pub_log_error("%s, %d, mtype_delete error, i[%d] result[%d]."
				, __FILE__, __LINE__, i, result);
			return SW_ERROR;
		}
	}

	/*test get max_type, cur_cnt*/
	pub_log_info("%s, %d, max_mtype[%d] cur_cnt[%d]"
			, __FILE__, __LINE__, mtype_get_max(), mtype_cur_cnt());

	/*test up-bound*/
	result = mtype_delete(count, 0);
	if (result != SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_delete error, mtype[200] result[%d]."
			, __FILE__, __LINE__, result);
		return SW_ERROR;
	}

	/*test below-bound*/
	result = mtype_delete(0, 0);
	if (result != SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_delete error, mtype[200] result[%d]."
			, __FILE__, __LINE__, result);
		return SW_ERROR;
	}

	/*test below-bound*/
	result = mtype_delete(-1, 0);
	if (result != SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_delete error, mtype[200] result[%d]."
			, __FILE__, __LINE__, result);
		return SW_ERROR;
	}

	mtype = mtype_new();
	if (mtype == SW_ERROR)
	{
		pub_log_error("%s, %d, mtype_new error, i[%d] mtype[%d]."
			, __FILE__, __LINE__, i, mtype);
		return SW_ERROR;
	}

	/*test save link info*/
	result = mtype_save_link_info(mtype, 9527, 9526, "pkg_key", "20130604", "tcpla1", 120);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_save_link_info error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test load mtype by key.*/
	sw_int64_t	bi_no = -1;
	sw_int64_t	trace_no = -1;
	sw_char_t	sys_date[15];
	pub_mem_memzero(sys_date, sizeof(sys_date));
	
	result = mtype_load_by_key("pkg_key", &bi_no, &trace_no, &mtype, sys_date);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_load_by_key error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (bi_no != 9527 || trace_no != 9526
		|| strcmp(sys_date, "20130604") != 0)
	{
		pub_log_error("%s, %d, mtype_load_by_key error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, bi_no[%lld] trace_no[%lld] mtype[%d] sys_date[%s]"
			,__FILE__, __LINE__, bi_no, trace_no, mtype, sys_date);

	/*test get lsn name*/
	sw_char_t	lsn_name[32];
	pub_mem_memzero(lsn_name, sizeof(lsn_name));
	result = mtype_get_lsn(mtype, lsn_name);
	if (result != SW_OK || strcmp(lsn_name, "tcpla1") != 0)
	{
		pub_log_error("%s, %d, mtype_get_lsn error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}
	
	pub_log_info("%s, %d, mtype[199] lsn_name[%s]", __FILE__, __LINE__, lsn_name);

	/*test set date*/
	result = mtype_set_date(mtype, 888, "20100405");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_date error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test get date by mtype*/
	trace_no = -1;
	pub_mem_memzero(sys_date, sizeof(sys_date));
	result = mtype_get_date(mtype, &trace_no, sys_date);
	if (result != SW_OK || trace_no != 888 || strcmp(sys_date, "20100405") != 0)
	{
		pub_log_error("%s, %d, mtype_get_date error.", __FILE__, __LINE__);
		return SW_ERROR;	
	}
	
	pub_log_info("%s, %d, mtype[199] trace_no[%lld] sys_date[%s] "
			, __FILE__, __LINE__, trace_no, sys_date);

	/*test set task info*/
	sw_task_info_t	task_info;
	pub_mem_memzero(&task_info, sizeof(task_info));
	task_info.err_info.err_code1 = 3;
	task_info.err_info.err_code2 = 4;
	sprintf(task_info.err_info.err_msg, "Oh god, a error message!");
	task_info.err_info.mode_num = 5;
	task_info.err_info.mode_type = 6;
	task_info.status = 7;
	task_info.trace_no = 887;

	result = mtype_set_task_info(mtype, &task_info);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_task_info error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test get task info*/
	pub_mem_memzero(&task_info, sizeof(task_info));
	result = mtype_get_task_info(mtype, &task_info);
	if (result != SW_OK || task_info.err_info.err_code1 != 3 
		|| strcmp(task_info.err_info.err_msg, "Oh god, a error message!") != 0
		|| task_info.err_info.err_code2 != 4 || task_info.err_info.mode_num != 5
		|| task_info.err_info.mode_type != 6 || task_info.status != 7
		|| task_info.trace_no != 887)
	{
		pub_log_error("%s, %d, mtype_get_task_info error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, mtype[199] err_code1[%d] err_msg[%s] err_code2[%d] mode_num[%d] mode_type[%d] status[%d] trace_no[%lld]"
			, __FILE__, __LINE__
			, task_info.err_info.err_code1, task_info.err_info.err_msg
			, task_info.err_info.err_code2, task_info.err_info.mode_num
			, task_info.err_info.mode_type, task_info.status
			, task_info.trace_no);

	/*test update mtype error.*/
	result = mtype_set_err(1, mtype, 9,  10, 11, 12, "another error message!");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_get_task_addr error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test get task info addr*/
	sw_task_info_t	*task_info_addr = NULL;
	task_info_addr = mtype_get_task_addr(mtype);
	if (task_info_addr == NULL 
		|| task_info_addr->err_info.err_code1 != 11 
		|| strcmp(task_info_addr->err_info.err_msg, "another error message!") != 0
		|| task_info_addr->err_info.err_code2 != 12 || task_info_addr->err_info.mode_num != 10
		|| task_info_addr->err_info.mode_type != 9 || task_info_addr->status != 7
		|| task_info_addr->trace_no != 887)
	{
		pub_log_error("%s, %d, mtype_get_task_addr error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_log_info("%s, %d, task_info_addr[%p] mtype[199] err_code1[%d] err_msg[%s] err_code2[%d] mode_num[%d] mode_type[%d] status[%d] trace_no[%lld]"
			, __FILE__, __LINE__ ,task_info_addr
			, task_info_addr->err_info.err_code1, task_info_addr->err_info.err_msg
			, task_info_addr->err_info.err_code2, task_info_addr->err_info.mode_num
			, task_info_addr->err_info.mode_type, task_info_addr->status
			, task_info_addr->trace_no);

	/*test set server service*/
	result = mtype_set_svr_svc(mtype, 8, "test_server", "test_service");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_get_task_addr error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test get server service*/
	sw_char_t	server[32];
	sw_char_t	service[32];
	result = mtype_get_svr_svc(mtype, server, service);
	if (result != 8 || strcmp(server, "test_server") != 0
		|| strcmp(service, "test_service") != 0 )
	{
		pub_log_error("%s, %d, mtype_get_svr_svc error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, mtype[199] server[%s] service[%s]"
			,__FILE__, __LINE__, server, service);

	/*test set status*/
	result = mtype_set_status(mtype, 1, -1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_status error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	/*test get status*/
	sw_int32_t	status = -1;
	sw_int32_t	task_status = -1;
	result = mtype_get_status(mtype, &status, &task_status);
	if (result != SW_OK || status != 1 || task_status != -1)
	{
		pub_log_error("%s, %d, mtype_get_status error.", __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, status[%d] task_status[%d]"
			, __FILE__, __LINE__, status, task_status);

	/*test check timeout*/
	sleep(2);
	result = mtype_check_timeout(mtype, 1);
	if (result != TASK_TIMEOUT)
	{
		pub_log_error("%s, %d, mtype_check_timeout error."
				, __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	pub_log_info("%s, %d, mtype[199] TASK_TIMEOUT[%d]"
			, __FILE__, __LINE__, result);

	result = mtype_check_timeout(mtype, 3);
	if (result == TASK_TIMEOUT)
	{
		pub_log_error("%s, %d, mtype_check_timeout error."
				, __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, mtype[199] TASK_TIMEOUT[%d]"
			, __FILE__, __LINE__, result);

	/*test get timeout*/
	result = mtype_get_timeout(mtype);
	if (result == SW_ERROR || result != 120)
	{
		pub_log_error("%s, %d, mtype_get_timeout error."
				, __FILE__, __LINE__);
		return SW_ERROR;			
	}

	pub_log_info("%s, %d, mtype[199] timeout[%d]"
			, __FILE__, __LINE__, result);

	/*test set saf flow*/
	result = mtype_set_saf_flow(mtype, "saf flow fail.", 130);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_saf_flow error."
				, __FILE__, __LINE__);
		return SW_ERROR;		
	}

	/*test get saf flow*/
	sw_char_t	saf_flow[128];
	pub_mem_memzero(saf_flow, sizeof(saf_flow));
	sw_int32_t	timeout = -1;
	result = mtype_get_saf_flow(mtype, saf_flow, &timeout);
	if (result != SW_OK || strcmp(saf_flow, "saf flow fail.") != 0 || timeout != 130)
	{
		pub_log_error("%s, %d, mtype_get_saf_flow error."
				, __FILE__, __LINE__);
		return SW_ERROR;		
	}

	pub_log_info("%s, %d, mtype[199] saf_flow[%s] timeout[%d]"
			, __FILE__, __LINE__, saf_flow, timeout);


	/*test set response*/
	result = mtype_set_resp(mtype, "0100", "0200");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_set_resp error."
				, __FILE__, __LINE__);
		return SW_ERROR;
	}

	/*test get response*/
	sw_char_t	resp[6];
	sw_char_t	sys_resp[6];
	
	result = mtype_get_resp(mtype, resp, sys_resp);
	if (result != SW_OK || strcmp(resp, "0100") != 0 || strcmp(sys_resp, "0200") != 0)
	{
		pub_log_error("%s, %d, mtype_set_resp error."
				, __FILE__, __LINE__);
		return SW_ERROR;	
	}

	pub_log_info("%s, %d, mtype[199] resp[%s] sys_resp[%s]"
			,__FILE__, __LINE__, resp, sys_resp);

	/*test get shm mtype head*/
	sw_mtype_head_t	head;
	pub_mem_memzero(&head, sizeof(head));
	result = mtype_get_head(&head);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, mtype_get_head error."
				, __FILE__, __LINE__);
		return SW_ERROR;	
	}

	pub_log_info("%s, %d, mtype[199] lock_id[%d] cur_cnt[%d] cur_use[%d] fail_cnt[%d] msg_cnt[%d] mtype_max[%d] size[%d] succ_cnt[%d] trc_cnt[%d] trc_fail_cnt[%d]"
			,__FILE__, __LINE__, head.lock_id, head.cur_cnt, head.cur_use
			, head.fail_cnt, head.msg_cnt, head.mtype_max
			, head.size, head.succ_cnt, head.trc_cnt, head.trc_fail_cnt);

	return SW_OK;
}

