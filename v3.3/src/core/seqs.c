#include "seqs.h"
#include "sem_lock.h"
#include "pub_log.h"
#include "pub_mem.h"
#include "pub_time.h"
#include "pub_xml.h"
#include "pub_ares.h"
#include "anet.h"
#include "alert.h"

extern int g_workmode_mp;
SW_PROTECTED sw_seqs_t *g_shm_seqs = NULL;
SW_PROTECTED sw_int_t seqs_loc_add_business_trace(sw_seqs_t *shm_seqs, const sw_seqcfg_t *seqcfg);
static sw_int_t seqs_print_flow(sw_char_t *buf);

SW_PROTECTED void
set_sequnit_by_seqcfg(sw_seqs_unit_t *u, const sw_seqcfg_t *seqcfg, sw_int32_t lockid)
{
	pub_mem_memzero(u, sizeof(sw_seqs_unit_t));

	strncpy(u->name, seqcfg->sqname, SW_NAME_LEN-1);
	u->lock_id = lockid;

	snprintf(u->prefix, sizeof(u->prefix), "%s", seqcfg->sqprefix);
	strcpy(u->trn_prefix, u->prefix);

	u->trn_base  = atoll(seqcfg->sqstart);
	u->base = u->trn_base;
	u->base_increment = atoi(seqcfg->sqstep);
	u->ejfno_max = atoll(seqcfg->sqmax);
	u->cycle = (sw_uchar_t)seqcfg->sqcycle;
}

SW_PROTECTED sw_int_t 
seqs_loc_init(sw_seqs_t *shm_seqs, const sw_syscfg_t* syscfg)
{
	sw_char_t	date[DATE_MAX_LEN];
	sw_int32_t      sem_lock_id;
	sw_seqcfg_t     dft_bpseq_cfg; /* dft, default */

	pub_mem_memzero(shm_seqs, sizeof(sw_seqs_t));

	shm_seqs->head.lock_id = sem_new_lock_id();
	if (shm_seqs->head.lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id error.", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sem_lock_id = sem_new_lock_id();
	if (sem_lock_id == -1)
	{
		pub_log_error("%s, %d, sem_new_lock_id error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	pub_mem_memzero(date, sizeof(date));
	pub_time_getdate(date, 1);
	strncpy(shm_seqs->head.sys_date, date, DATE_MAX_LEN-1);
	strncpy(shm_seqs->head.vfs_date, date, DATE_MAX_LEN-1);
	shm_seqs->head.date_lock_flag = SEQS_DATE_UNLOCK;
	shm_seqs->head.sys_status = SEQS_SYS_STAT_RUN;

	memset(&dft_bpseq_cfg, 0x00, sizeof(sw_seqcfg_t));
	snprintf(dft_bpseq_cfg.sqname, sizeof dft_bpseq_cfg.sqname, "DEFAULT_BP");
	strcpy(dft_bpseq_cfg.sqstart, "1");
	strcpy(dft_bpseq_cfg.sqstep, "1");
	strcpy(dft_bpseq_cfg.sqprefix, "0");
	strcpy(dft_bpseq_cfg.sqmax, "99999999");
	dft_bpseq_cfg.sqcycle= SEQ_CYCLE_ENABLE;

	sw_seqs_unit_t *u = &shm_seqs->bp_trc_array[0]; /* default unit for compatibility */
	set_sequnit_by_seqcfg(u, &dft_bpseq_cfg, sem_lock_id);

	shm_seqs->head.busi_trc_cnt = 0;
	shm_seqs->head.bp_trc_cnt = 1;
	shm_seqs->head.max_seq = syscfg->seq_max;
	
	/*** INIT SEQS ***/
	{
		sw_int_t	ret = 0;
		char	xmlname[128];
		sw_xmltree_t	*xml = NULL;
		sw_xmlnode_t	*node = NULL;
		sw_xmlnode_t	*node1 = NULL;
		sw_seqcfg_t	seqcfg;
		
		memset(xmlname, 0x0, sizeof(xmlname));
		sprintf(xmlname, "%s/cfg/swconfig.xml", getenv("SWWORK"));
		xml = pub_xml_crtree(xmlname);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
			return SW_ERROR;
		} 
	
		node = pub_xml_locnode(xml, ".SWCFG.SEQN");
		while (node != NULL)
		{
			if (strcmp(node->name, "SEQN") != 0)
			{
				node = node->next;
				continue;
			}
			
			memset(&seqcfg, 0x0, sizeof(seqcfg));
			xml->current = node;
			node1 = pub_xml_locnode(xml, "SQNAME");
			if (node1 == NULL || node1->value == NULL)
			{
				pub_log_error("[%s][%d] Not config SQNAME!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return -1;
			}
			strncpy(seqcfg.sqname, node1->value, sizeof(seqcfg.sqname) - 1);
			
			node1 = pub_xml_locnode(xml, "SQPREFIX");
			if (node1 != NULL && node1->value != NULL)
			{
				snprintf(seqcfg.sqprefix, sizeof(seqcfg.sqprefix),  "%s", node1->value);
			}
			else
			{
				strcpy(seqcfg.sqprefix, "0");
			}
			
			pub_log_debug("[%s][%d] workmode_mp====[%d]", __FILE__, __LINE__, g_workmode_mp);
			if (g_workmode_mp)
			{	
				node1 = pub_xml_locnode(xml, "SQBUSPREFIX");
				if (node1 == NULL || node1->value == NULL)
				{
					pub_log_error("[%s][%d] MP workmode must config SQBUSPREFIX!", __FILE__, __LINE__);
					pub_xml_deltree(xml);
					return -1;
				}
				snprintf(seqcfg.sqprefix, sizeof(seqcfg.sqprefix),  "%s", node1->value);
				pub_log_info("[%s][%d] prefix=[%s]", __FILE__, __LINE__, seqcfg.sqprefix);
			}

			node1 = pub_xml_locnode(xml, "SQSTART");
			if (node1 != NULL && node1->value != NULL)
			{
				snprintf(seqcfg.sqstart, sizeof(seqcfg.sqstart), "%ld", atol(node1->value));
			}
			else
			{
				snprintf(seqcfg.sqstart, sizeof(seqcfg.sqstart), "1");
			}
			
			node1 = pub_xml_locnode(xml, "SQSTEP");
			if (node1 != NULL && node1->value != NULL)
			{
				strncpy(seqcfg.sqstep, node1->value, sizeof(seqcfg.sqstep) - 1);
			}
			else
			{
				strcpy(seqcfg.sqstep, "1");
			}
			
			node1 = pub_xml_locnode(xml, "SQMAX");
			if (node1 != NULL && node1->value != NULL)
			{
				snprintf(seqcfg.sqmax, sizeof(seqcfg.sqmax), "%lld", atoll(node1->value));
			}
			else
			{
				snprintf(seqcfg.sqmax, sizeof(seqcfg.sqmax), "99999999");
			}
			
			seqcfg.sqcycle = SEQ_CYCLE_ENABLE;

			ret = seqs_loc_add_business_trace(shm_seqs, &seqcfg);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] add trace error!", __FILE__, __LINE__);
				pub_xml_deltree(xml);
				return SW_ERROR;
			}
		
			node = node->next;
			pub_log_info("[%s][%d] 初始化[%s]完成!", __FILE__, __LINE__, seqcfg.sqname);
		}
		pub_xml_deltree(xml);
	}

	return SW_OK;
}

sw_int_t seqs_init(sw_seqs_t *shm_seqs, const sw_syscfg_t* syscfg)
{
	g_shm_seqs = shm_seqs;
	return  seqs_loc_init(shm_seqs, syscfg);
}


sw_int32_t seqs_get_size(sw_syscfg_t *syscfg)
{
	/*  (seq_max -1): because sw_seqs_t has a member of sw_busi_t */
	return sizeof(sw_seqs_t) + (syscfg->seq_max - 1)*sizeof(sw_seqs_unit_t);
}


sw_int_t seqs_set_addr(const sw_char_t* addr)
{
	g_shm_seqs = (sw_seqs_t *)addr;
	return SW_OK;
}


SW_PROTECTED sw_int_t 
seqs_loc_add_business_trace(sw_seqs_t *shm_seqs, const sw_seqcfg_t *seqcfg)
{
	sem_write_lock(shm_seqs->head.lock_id);

	if (shm_seqs->head.busi_trc_cnt > shm_seqs->head.max_seq)
	{
		pub_log_error("[%s][%d] Error: busi_trc_cnt=[%d] max_seq=[%d]", 
			__FILE__, __LINE__, shm_seqs->head.busi_trc_cnt, shm_seqs->head.max_seq);
	
		sem_write_unlock(shm_seqs->head.lock_id);
		return SW_ERROR;
	}

	sw_int32_t	i;
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		if (strcmp(shm_seqs->busi_trc_array[i].name, seqcfg->sqname) == 0)
		{
			pub_log_info("%s, %d, business seqs[%s] exist."
					, __FILE__, __LINE__, seqcfg->sqname);
			sem_write_unlock(shm_seqs->head.lock_id);
			return SW_OK;
		}
	}

	sw_seqs_unit_t *u = &shm_seqs->busi_trc_array[shm_seqs->head.busi_trc_cnt];
	sw_int32_t lockid = sem_new_lock_id();
	if (lockid == -1)
	{
		pub_log_error("[%s][%d] sem_new_lock_id error!", __FILE__, __LINE__);
		sem_write_unlock(shm_seqs->head.lock_id);
		return SW_ERROR;
	}

	set_sequnit_by_seqcfg(u, seqcfg, lockid);
	shm_seqs->head.busi_trc_cnt += 1;
	sem_write_unlock(shm_seqs->head.lock_id);
	pub_log_info("[%s][%d] Add [%s] success!", __FILE__, __LINE__, seqcfg->sqname);

	return SW_OK;
}


sw_int_t seqs_add_business_trace(const char* name, sw_int64_t prefix)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));

		len = snprintf(buf, sizeof(buf), "%s%s%lld", name, ARES_SEP, prefix);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_ADD_BUSINESS_TRACE);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares add business trace error! name=[%s]",
					__FILE__, __LINE__, name);
		}
	}

	sw_seqcfg_t seqcfg;

	memset(&seqcfg, 0x00, sizeof(seqcfg));
	snprintf(seqcfg.sqname,   sizeof seqcfg.sqname,    "%s",   name);
	snprintf(seqcfg.sqprefix, sizeof seqcfg.sqprefix,  "%lld", (long long)prefix);
	snprintf(seqcfg.sqstart, sizeof seqcfg.sqstart , "%lld000000001", (long long)prefix);
	strcpy(seqcfg.sqstep, "1");
	snprintf(seqcfg.sqmax, sizeof seqcfg.sqmax, "%lld%lld", (long long)prefix, 999999999LL);

	return (seqs_loc_add_business_trace(g_shm_seqs, &seqcfg));
}

SW_PROTECTED sw_int64_t 
get_no_from_unit(sw_seqs_unit_t *u, sw_char_t *trace_no)
{
	sem_write_lock(u->lock_id);
	sprintf(trace_no, "%s%0*lld", u->trn_prefix, (int)(SW_TRACE_LEN - strlen(u->trn_prefix)), u->trn_base);
	if (u->trn_base > u->ejfno_max)
	{
		switch (u->cycle)
		{
			case SEQ_CYCLE_ENABLE:
				u->trn_base = u->base;
				sem_write_unlock(u->lock_id);
				return get_no_from_unit(u, trace_no);
			case SEQ_CYCLE_DISABLE:
				sem_write_unlock(u->lock_id);
				return -1;
			case SEQ_CYCLE_DB_GET:
				sem_write_unlock(u->lock_id);
				return -2; /* DB shou call seqs_reset_bp(bsn)_trace() */
			default:
				sem_write_unlock(u->lock_id);
				/* assert(0);  */
				return -1;
		}
	}

	u->trn_base += u->base_increment;
	sem_write_unlock(u->lock_id);

	return 0;
}

sw_int64_t seqs_loc_new_trace_no(sw_seqs_t *shm_seqs, const char *name, sw_char_t *trace_no)
{
	sw_seqs_unit_t  *u;
	sw_int_t        index;

	if (name == NULL)
	{
		index = 0;
	}
	else
	{
		sem_read_lock(shm_seqs->head.lock_id);
		for (index = 0; index < shm_seqs->head.bp_trc_cnt; index++)
		{
			if (0 == strcmp(name, shm_seqs->bp_trc_array[index].name))
				break;
		}
		sem_read_unlock(shm_seqs->head.lock_id);

		if(index == shm_seqs->head.bp_trc_cnt)
			return SW_ERROR;
	}

	u = &(shm_seqs->bp_trc_array[index]);
	return get_no_from_unit(u, trace_no);
}


sw_int64_t seqs_new_trace_no()
{
	sw_int_t	ret = 0;
	sw_char_t	trace_no[SQN_LEN];

	memset(trace_no, 0x0, sizeof(trace_no));

	if (g_seqs_in_ares)
	{
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_BP_NEW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get bp traceno error!",__FILE__, __LINE__);
			goto END;
		}
		strncpy(trace_no, buf, sizeof(trace_no) - 1);

		return atoll(trace_no);
	}
END:
	ret = seqs_loc_new_trace_no(g_shm_seqs, NULL, trace_no);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get new trace no error!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return atoll(trace_no);
}

SW_PROTECTED sw_int64_t 
seqs_loc_get_trace_no(sw_seqs_t *shm_seqs, sw_char_t *trace_no)
{
	sw_seqs_unit_t  *first_unit;

	first_unit = &(shm_seqs->bp_trc_array[0]);

	sem_read_lock(first_unit->lock_id);
	sprintf(trace_no, "%s%0*lld", first_unit->trn_prefix, (int)(SW_TRACE_LEN - strlen(first_unit->trn_prefix)), first_unit->trn_base);
	sem_read_unlock(first_unit->lock_id);

	return 0;
}


sw_int64_t seqs_get_trace_no()
{
	sw_char_t	trace_no[SQN_LEN];

	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	memset(trace_no, 0x0, sizeof(trace_no));
	seqs_loc_get_trace_no(g_shm_seqs, trace_no);

	return atoll(trace_no);
}

sw_int_t seqs_get(sw_seqs_head_t* seqhead)
{
	sw_seqs_t *shm_seqs = g_shm_seqs;

	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	sem_read_lock(shm_seqs->head.lock_id);

	/* I am confused about the sizeof part. After grep some
	 * code that calling this func, I am sure of it. */
	pub_mem_memcpy(seqhead, shm_seqs, sizeof(sw_seqs_head_t));
	sem_read_unlock(shm_seqs->head.lock_id);

	return SW_OK;
}


SW_PROTECTED sw_int64_t 
seqs_loc_new_business_no(sw_seqs_t *shm_seqs, const char* name, int step, sw_char_t *trace_no)
{
	sw_int32_t	i = 0;

	if (name == NULL || name[0] == '\0')
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_read_lock(shm_seqs->head.lock_id);

	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		if (strcmp(shm_seqs->busi_trc_array[i].name, name) == 0)
		{
			sw_seqs_unit_t *u = &(shm_seqs->busi_trc_array[i]);

			get_no_from_unit(u, trace_no);

			sem_write_lock(u->lock_id);
			if (step >= 0)
			{
				u->trn_base -= u->base_increment; /* restore */
				u->trn_base += step;
			}
			sem_write_unlock(u->lock_id);

			sem_read_unlock(shm_seqs->head.lock_id);
			return  0;
		}
	}
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		pub_log_info("[%s][%d] busi_trc_array[%d].name=[%s] name=[%s]",
				__FILE__, __LINE__, i, shm_seqs->busi_trc_array[i].name, name);
	}
	sem_read_unlock(shm_seqs->head.lock_id);

	return SW_ERROR;
}

/******************************************************************************
 ** Name : seqs_new_business_no
 ** Desc : 申请业务流水号
 ** Input: 
 **     name: 业务流水名
 **     step: 申请的业务水流个数
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Wangkun # 2013.06.29 #
 ******************************************************************************/
sw_int64_t seqs_new_business_no(const char* name, int step, sw_char_t *trace_no)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%s%s%d", name, ARES_SEP, step);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_BUS_NEW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get new business trace error! name=[%s] step=[%d]",
				__FILE__, __LINE__, name, step);
			goto END;
		}
		strcpy(trace_no, buf);

		return SW_OK;
	}

END:
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_new_business_no(g_shm_seqs, name, step, trace_no));
}

SW_PROTECTED sw_int32_t
seqs_loc_get_syssts(sw_seqs_t *shm_seqs)
{
	sw_int32_t	status = 0;

	if (shm_seqs == NULL)
	{
		pub_log_error("[%s][%d] shm_seqs is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	sem_read_lock(shm_seqs->head.lock_id);
	status = shm_seqs->head.sys_status;
	sem_read_unlock(shm_seqs->head.lock_id);

	return status;
}

sw_int32_t seqs_get_syssts()
{
	if (g_seqs_in_ares)
	{
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_GET_SYSSTS);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get sys status error!",
				__FILE__, __LINE__);
			goto END;
		}
		pub_log_info("[%s][%d] buf=[%s]", __FILE__, __LINE__, buf);
		return atoi(buf);  
	}

END:
	if (g_shm_seqs == NULL)
	{
		pub_log_error("[%s][%d] g_shm_seqs is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_get_syssts(g_shm_seqs));
}

SW_PROTECTED sw_int32_t
seqs_loc_set_syssts(sw_seqs_t *shm_seqs, sw_int32_t status)
{
	if (shm_seqs == NULL)
	{
		pub_log_error("[%s][%d] shm_seqs is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (status != SEQS_SYS_STAT_RUN && status != SEQS_SYS_STAT_STOP)
	{
		pub_log_error("[%s][%d] status [%d] is error!", __FILE__, __LINE__, status);
		return SW_ERROR;
	}

	sem_write_lock(shm_seqs->head.lock_id);
	shm_seqs->head.sys_status = status;
	sem_write_unlock(shm_seqs->head.lock_id);

	return SW_OK;
}

SW_PROTECTED sw_int32_t seqs_set_syssts(sw_int32_t status)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%d", status);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_SET_SYSSTS);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set sys status error!",
					__FILE__, __LINE__);
		}
	}
	if (g_shm_seqs == NULL)
	{
		pub_log_error("[%s][%d] g_shm_seqs is null!", __FILE__, __LINE__);
		return SW_ERROR;
	}

	return seqs_loc_set_syssts(g_shm_seqs, status);
}

sw_int32_t seqs_sys_start()
{
	return seqs_set_syssts(SEQS_SYS_STAT_RUN);
}

sw_int32_t seqs_sys_stop()
{
	return seqs_set_syssts(SEQS_SYS_STAT_STOP);
}

SW_PROTECTED sw_int_t 
seqs_loc_get_sysdate(sw_seqs_t *shm_seqs, char *sysdate, int size)
{
	if (shm_seqs == NULL)
	{
		pub_log_error("%s, %d, shm_seqs == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (sysdate == NULL)
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_read_lock(shm_seqs->head.lock_id);
	strncpy(sysdate, shm_seqs->head.sys_date, size);
	sem_read_unlock(shm_seqs->head.lock_id);

	sysdate[size-1] = 0;

	return SW_OK;
}


sw_int_t seqs_get_sysdate(char *sysdate)
{
	if (g_seqs_in_ares)
	{
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_GET_SYSDATE);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get sysdate error!",
					__FILE__, __LINE__);
			goto END;
		}
		strcpy(sysdate, buf);

		return SW_OK;
	}
END:
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_get_sysdate(g_shm_seqs, sysdate, DATE_MAX_LEN));
}


sw_int_t seqs_get_curdate(char *sysdate)
{
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_get_sysdate(g_shm_seqs, sysdate, DATE_MAX_LEN));
}


sw_int_t seqs_get_date(char *date, int size)
{
	if (g_seqs_in_ares)
	{
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_GET_DATE);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get date error!",
					__FILE__, __LINE__);
			goto END;
		}
		snprintf(date, size, "%s", buf);

		return SW_OK;
	}
END:
	return (seqs_loc_get_sysdate(g_shm_seqs, date, size));
}


sw_int_t  seqs_is_date_locked(void)
{
	sw_seqs_t *shm_seqs = g_shm_seqs;
	sw_int_t is_locked = -1;

	if (g_seqs_in_ares)
	{
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_IS_DATE_LOCKED);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get date lock status error!",
					__FILE__, __LINE__);
			goto END;
		}
		return (buf[0] - '0');
	}
END:

	sem_read_lock(shm_seqs->head.lock_id);
	if (shm_seqs->head.date_lock_flag == SEQS_DATE_LOCK)
		is_locked = 1;
	else if (shm_seqs->head.date_lock_flag == SEQS_DATE_UNLOCK)
		is_locked = 0;
	sem_read_unlock(shm_seqs->head.lock_id);

	if (is_locked == -1)
	{
		pub_log_error("[%s][%d] Error!!!!", __FILE__, __LINE__);
	}
	return is_locked;
}


SW_PROTECTED sw_int_t 
seqs_loc_change_date(sw_seqs_t *shm_seqs, const char *new_sysdate)
{
	sw_int32_t	i = 0;

	if (shm_seqs == NULL)
	{
		pub_log_error("%s, %d, shm_seqs == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (new_sysdate == NULL)
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_read_lock(shm_seqs->head.lock_id);
	if (SEQS_DATE_LOCK == shm_seqs->head.date_lock_flag)
	{
		sem_read_unlock(shm_seqs->head.lock_id);
		pub_log_error("%s, %d, date is LOCKED, can not change", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sem_read_unlock(shm_seqs->head.lock_id);
	sem_write_lock(shm_seqs->head.lock_id);

	/*set sysdate*/ 
	strncpy(shm_seqs->head.sys_date, new_sysdate, DATE_MAX_LEN);
	shm_seqs->head.sys_date[DATE_MAX_LEN-1] = 0;

	pub_log_info("seq date has been changed to (%s) according to(%s) ", 
			shm_seqs->head.sys_date, new_sysdate);


	/*reset trace no*/
	for (i = 0; i < MAX_SEQS_CNT; i++)
	{
		sw_seqs_unit_t *u = &shm_seqs->bp_trc_array[i];

		sem_write_lock(u->lock_id);
		u->trn_base = u->base;
		sem_write_unlock(u->lock_id);
	}
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		sw_seqs_unit_t *u = &shm_seqs->busi_trc_array[i];

		sem_write_lock(u->lock_id);
		u->trn_base = u->base;
		sem_write_unlock(u->lock_id);
	}		

	sem_write_unlock(shm_seqs->head.lock_id);

	return SW_OK;
}

/******************************************************************************
 ** Name : seqs_change_date
 ** Desc : 日切
 ** Input: 
 **     new_sysdate: 指定的日切日期. 格式: YYYYMMDD
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Wangkun # 2013.06.29 #
 ******************************************************************************/
sw_int_t seqs_change_date(const char *new_sysdate)
{
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_change_date(g_shm_seqs, new_sysdate));
}

/******************************************************************************
 ** Name : seqs_set_bsn_flow
 ** Desc : 设置业务流水号(Set business flow no)
 ** Input: 
 **     name: 业务水流名(Name of business generator)
 **     flwno: 业务流水号(Flow No. of business)
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : set -pf [flwno]
 ** Author: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
sw_int_t seqs_set_bsn_flow(const char *name, sw_int64_t flwno)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%s%s%lld", name, ARES_SEP, flwno);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_SET_BSN_FLOW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set business flow error! name=[%s]",
					__FILE__, __LINE__, name);
		}
	}

	sw_int_t idx = 0;
	sw_seqs_t *shm_seqs = g_shm_seqs;

	sem_read_lock(shm_seqs->head.lock_id);
	for(idx=0; idx<shm_seqs->head.busi_trc_cnt; idx++)
	{
		if(0 == strcmp(shm_seqs->busi_trc_array[idx].name, name))
		{
			sw_seqs_unit_t *u = &(shm_seqs->busi_trc_array[idx]);

			sem_write_lock(u->lock_id);

			if (flwno > u->ejfno_max) 
			{
				sem_write_unlock(u->lock_id);
				sem_read_unlock(shm_seqs->head.lock_id);
				pub_log_error("[%s][%d] Flow(%d) no is out of range!",
						__FILE__, __LINE__, flwno);
				return SW_ERR;
			}

			/* what if flwno < u.base ? Should we return error on it ? */
			u->trn_base = flwno;

			sem_write_unlock(u->lock_id);
			sem_read_unlock(shm_seqs->head.lock_id);
			return SW_OK;
		}
	}

	sem_read_unlock(shm_seqs->head.lock_id);
	return SW_ERR;
}


sw_int_t seqs_reset_bsn_trace(const char *name, const sw_seqcfg_t *seqcfg)
{
	sw_int32_t	i;
	sw_seqs_t *shm_seqs = g_shm_seqs;

	sem_read_lock(shm_seqs->head.lock_id);
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		if (strcmp(shm_seqs->busi_trc_array[i].name, seqcfg->sqname) == 0)
			break;
	}
	sem_read_unlock(shm_seqs->head.lock_id);

	if (i == shm_seqs->head.busi_trc_cnt)
		return SW_ERROR;;

	sw_seqs_unit_t *u = &shm_seqs->busi_trc_array[i];

	sem_write_lock(u->lock_id);
	set_sequnit_by_seqcfg(u, seqcfg, u->lock_id);
	sem_write_unlock(u->lock_id);

	return SW_OK;
}

/******************************************************************************
 ** Name : seqs_set_plat_flow
 ** Desc : 设置平台流水号
 ** Input: 
 **     flwno: 业务流水号
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
sw_int_t seqs_set_plat_flow_help(int index, sw_int64_t flwno)
{
	sw_seqs_t *shm_seqs = g_shm_seqs;
	sw_seqs_unit_t *u = &(shm_seqs->bp_trc_array[index]);

	if(shm_seqs == NULL)
		return SW_ERR;

	sem_write_lock(u->lock_id);

	if (flwno > u->ejfno_max)
	{
		sem_write_unlock(u->lock_id);
		pub_log_error("[%s][%d] Flow(%d) no is out of range!",
				__FILE__, __LINE__, flwno);
		return SW_ERR;
	}

	u->trn_base = flwno;

	sem_write_unlock(u->lock_id);
	return SW_OK;

}


sw_int_t seqs_set_plat_flow(sw_int64_t flwno)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%lld", flwno);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_SET_PLAT_FLOW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set plat flow error!",
					__FILE__, __LINE__);
		}
	}
	return seqs_set_plat_flow_help(0, flwno);
}


sw_int_t seqs_set_bp_flow(const char *name, sw_int64_t flwno)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%s%s%lld", name, ARES_SEP, flwno);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_SET_BP_FLOW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set bp flow error!",
					__FILE__, __LINE__);
		}
	}
	sw_int_t index;
	sw_seqs_t *shm_seqs = g_shm_seqs;

	if(shm_seqs == NULL)
		return SW_ERR;

	sem_read_lock(shm_seqs->head.lock_id);
	for (index = 0; index < shm_seqs->head.bp_trc_cnt; index++)
	{
		if (0 == strcmp(name, shm_seqs->bp_trc_array[index].name))
			break;
	}
	sem_read_unlock(shm_seqs->head.lock_id);

	if(index != shm_seqs->head.bp_trc_cnt)
		return seqs_set_plat_flow_help(index, flwno);
	else
		return SW_ERROR;
}

/******************************************************************************
 ** Name : seqs_set_date_lock
 ** Desc : 设置日期计算起点
 ** Input: 
 **     new_sysdate: 指定的日切日期. 格式: YYYYMMDD
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 **     今后的时间以new_date为计算起点
 ** Author: # Qifeng.zou # 2013.06.30 #
 ******************************************************************************/
sw_int_t seqs_set_date_lock(const char *new_date)
{
	if (g_seqs_in_ares)
	{
		int	len = 0;
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%s", new_date);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_SET_DATE_LOCK);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set date error!", __FILE__, __LINE__);
		}
	}

	sw_seqs_t *shm_seqs = g_shm_seqs;

	if(g_shm_seqs == NULL || new_date == NULL)
		return SW_ERR;

	if (SW_OK != seqs_loc_change_date(shm_seqs, new_date))
		return SW_ERR;

	sem_write_lock(shm_seqs->head.lock_id);
	shm_seqs->head.date_lock_flag = SEQS_DATE_LOCK;
	sem_write_unlock(shm_seqs->head.lock_id);

	return SW_OK;
}

/******************************************************************************
 ** Name : seqs_set_date_unlock
 ** Desc : 解锁日期
 ** Input: 
 **     new_sysdate: 指定的日切日期. 格式: YYYYMMDD
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Qifeng.zou # 2013.06.29 #
 ******************************************************************************/
sw_int_t seqs_set_date_unlock(void)
{
	if (g_seqs_in_ares)
	{
		int	ret = 0;
		char	buf[128];

		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_SET_DATE_UNLOCK);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares set date unlock error!",
					__FILE__, __LINE__);
		}
	}

	sw_seqs_t *shm_seqs = g_shm_seqs;

	if(g_shm_seqs == NULL)
		return SW_ERR;

	sem_read_lock(shm_seqs->head.lock_id);
	if (shm_seqs->head.date_lock_flag == SEQS_DATE_UNLOCK)
	{
		sem_read_unlock(shm_seqs->head.lock_id);
	}
	else
	{
		sem_read_unlock(shm_seqs->head.lock_id);
		sem_write_lock(shm_seqs->head.lock_id);
		shm_seqs->head.date_lock_flag = SEQS_DATE_UNLOCK;
		sem_write_unlock(shm_seqs->head.lock_id);
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : seqs_print_plat_flow
 ** Desc : 打印平台流水的相关信息
 ** Input: NONE
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
sw_int_t seqs_print_plat_flow(void)
{
	int	i = 0;
	int	total = 0;
	sw_int_t	ret = 0;
	sw_int_t	status = SW_ERROR;
	sw_seqs_t	*shm_seqs = g_shm_seqs;
	sw_char_t	buf[MAX_BUF_LEN];

	fprintf(stderr, "\n\t%-4s %-32s %-12s %-12s %-12s\n", "No.", "NAME", "PREFIX", "BASE", "CURRENT");
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	if (g_seqs_in_ares)
	{
		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_PRINT_PLAT_FLOW_NEW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares print plat flow error!",
					__FILE__, __LINE__);
			status = SW_ERROR;
			goto end;
		}
		ret = seqs_print_flow(buf);
		if (ret < 0)
		{
			status = SW_ERROR;
			goto end;
		}
		total = ret;
		status = SW_OK;
	}
	else
	{
		sem_read_lock(shm_seqs->head.lock_id);
		for (i = 0; i < (shm_seqs->head.bp_trc_cnt); i++)
		{
			sw_seqs_unit_t *u = &shm_seqs->bp_trc_array[i];

			sem_read_lock(u->lock_id);
			fprintf(stderr, "\t[%02d] %-32s %-12s %-12lld %-12lld\n",
					i + 1,
					u->name,
					u->prefix,
					(long long)(u->base),
					(long long)(u->trn_base));
			sem_read_unlock(u->lock_id);
			total++;
		}
		sem_read_unlock(shm_seqs->head.lock_id);
		status = SW_OK;
	}
end:
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	fprintf(stderr, "\tTOTAL:%d\n\n", total);

	return status;
}

/******************************************************************************
 ** Name : seqs_print_bsn_flow
 ** Desc : 打印指定业务流水生成器信息
 ** Input: 
 **     bsn_name: 业务名
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
sw_int_t seqs_print_bsn_flow(const char *bsn_name)
{
	int	idx = 0; 
	int	len = 0;
	int	total = 0;
	sw_int_t	ret = 0;
	sw_int_t	status = SW_ERROR;
	sw_seqs_t	*shm_seqs = g_shm_seqs;
	sw_char_t	buf[MAX_BUF_LEN];

	fprintf(stderr, "\n\t%-4s %-32s %-12s %-12s %-12s\n", "No.", "NAME", "PREFIX", "BASE", "CURRENT");
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	if (g_seqs_in_ares)
	{
		memset(buf, 0x0, sizeof(buf));
		len = snprintf(buf, sizeof(buf), "%s", bsn_name);
		ret = ares_comm(g_ares_fd, buf, len, SEQS_PRINT_BSN_FLOW_NEW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares print business flow error! name=[%s]",
					__FILE__, __LINE__, bsn_name);
			status = SW_ERROR;
			goto end;
		}
		ret = seqs_print_flow(buf);
		if (ret < 0)
		{
			status = SW_ERROR;
			goto end;
		}
		status = SW_OK;
		total = ret;
	}
	else
	{
		sem_read_lock(shm_seqs->head.lock_id);
		for(idx=0; idx<shm_seqs->head.busi_trc_cnt; idx++)
		{
			if(0 == strcmp(shm_seqs->busi_trc_array[idx].name, bsn_name))
			{            
				sw_seqs_unit_t *u = &shm_seqs->busi_trc_array[idx];
				sem_read_lock(u->lock_id);
				fprintf(stderr, "\t[%02d] %-32s %-12s %-12lld %-12lld\n",
						idx + 1,
						u->name,
						u->prefix,
						(long long)(u->base),
						(long long)(u->trn_base));
				sem_read_unlock(shm_seqs->busi_trc_array[idx].lock_id);
				total++;

				break;
			}
		}
		sem_read_unlock(shm_seqs->head.lock_id);
		status = SW_OK;
	}
end:
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	fprintf(stderr, "\tTOTAL:%d\n\n", total);

	return status;
}

/******************************************************************************
 ** Name : seqs_print_bsn_flow_all
 ** Desc : 打印所有业务流水生成器信息
 ** Input: NONE
 ** Output: NONE
 ** Return: 0: success !0: failed
 ** Process:
 ** Note : 
 ** Author: # Qifeng.zou # 2013.07.10 #
 ******************************************************************************/
sw_int_t seqs_print_bsn_flow_all(void)
{
	int	idx = 0;
	int 	total = 0;
	sw_int_t	ret = 0;
	sw_int_t	status = SW_ERROR;
	sw_seqs_t	*shm_seqs = g_shm_seqs;
	sw_char_t	buf[MAX_BUF_LEN];

	fprintf(stderr, "\n\t%-4s %-32s %-12s %-12s %-12s\n", "No.", "NAME", "PREFIX", "BASE", "CURRENT");
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	if (g_seqs_in_ares)
	{
		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_PRINT_BSN_FLOW_ALL_NEW);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares print business all flow error!",
					__FILE__, __LINE__);
			status = SW_ERROR;
			goto end;
		}
		ret = seqs_print_flow(buf);
		if (ret < 0)
		{
			status = SW_ERROR;
			goto end;
		}
		status = SW_OK;
		total = ret;
	}
	else
	{
		sem_read_lock(shm_seqs->head.lock_id);
		for(idx=0; idx<shm_seqs->head.busi_trc_cnt; idx++)
		{           
			sw_seqs_unit_t *u = &shm_seqs->busi_trc_array[idx];
			sem_read_lock(u->lock_id);
			fprintf(stderr, "\t[%02d] %-32s %-12s %-12lld %-12lld\n",
					idx + 1,
					u->name,
					u->prefix,
					(long long)(u->base),
					(long long)(u->trn_base));
			sem_read_unlock(shm_seqs->busi_trc_array[idx].lock_id);
			total++;
		}
		sem_read_unlock(shm_seqs->head.lock_id);
		status = SW_OK;
	}
end:
	fprintf(stderr, "\t---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----\n");
	fprintf(stderr, "\tTOTAL:%d\n\n", total);

	return status;
}

sw_int_t seqs_loc_save(sw_seqs_t *shm_seqs, int flag)
{
	FILE	*fp = NULL;
	sw_char_t	filepath[512];
	sw_int32_t	i = 0;

	if (shm_seqs == NULL)
	{
		pub_log_error("%s, %d, shm_seqs == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	/* SEQS_SAVE_STATUS_ENUM starts from 1, so 0 is illegal */
	if (flag <= 0 || flag >= SEQS_SAVE_STAT_TOTAL)
	{
		pub_log_error("%s, %d, Param error", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if(getenv("SWWORK") == NULL)
	{
		pub_log_error("%s, %d, getenv(SWWORK) returns NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	if (snprintf(filepath, sizeof filepath, "%s/cfg/.TRACEFILE",
				getenv("SWWORK")) >= (int)sizeof filepath)
	{
		pub_log_error("%s, %d, buf too small", __FILE__, __LINE__);
		return SW_ERROR;
	}

	fp = fopen(filepath,"wb");
	if (fp == NULL)
	{
		pub_log_error("%s, %d, fopen[%s] error.",__FILE__,__LINE__,filepath);
		return SW_ERROR;
	}

	sw_int_t ret = SW_ERROR;

	sem_write_lock(shm_seqs->head.lock_id);
	for (i = 0; i < shm_seqs->head.bp_trc_cnt; i++)
		sem_write_lock(shm_seqs->bp_trc_array[i].lock_id);
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
		sem_write_lock(shm_seqs->busi_trc_array[i].lock_id);

	shm_seqs->head.status = flag;

	size_t size = sizeof(sw_seqs_t) + (shm_seqs->head.max_seq - 1) * sizeof(sw_seqs_unit_t);
	if(size != fwrite(shm_seqs, 1, size, fp))
	{
		pub_log_error("%s, %d, fwrite error.",__FILE__,__LINE__);
		goto end;
	}

	ret = SW_OK;
	pub_log_info("seqs(size %zu) saved to file", size);

end:
	if (fp)
	{
		fclose(fp);
	}

	for (i = 0; i < shm_seqs->head.bp_trc_cnt; i++)
		sem_write_unlock(shm_seqs->bp_trc_array[i].lock_id);
	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
		sem_write_unlock(shm_seqs->busi_trc_array[i].lock_id);

	sem_write_unlock(shm_seqs->head.lock_id);

	return ret;
}

sw_int_t seqs_save(int flag)
{
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_save(g_shm_seqs, flag));
}

sw_int_t seqs_loc_recover(sw_seqs_t *shm_seqs)
{
	FILE	*fp = NULL;
	char	*filebuf = NULL;
	sw_char_t	tmp[200];
	sw_int32_t	i = 0;
	sw_int32_t	size = 0;
	struct stat	filestat;

	if (shm_seqs == NULL)
	{
		pub_log_error("%s, %d, shm_seqs == NULL", __FILE__, __LINE__);
		return SW_ERROR;
	}

	sprintf(tmp, "%s/cfg/.TRACEFILE", getenv("SWWORK"));
	if (0 != stat(tmp, &filestat) || filestat.st_size == 0)
	{
		pub_log_error("[%s][%d] stat [%s] error, errno=[%d]:[%s]",
			__FILE__, __LINE__, tmp, errno, strerror(errno));
		goto ErrExit;
	}

	fp = fopen(tmp, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] Can not open [%s], errno=[%d]:[%s]",
			__FILE__, __LINE__, tmp, errno, strerror(errno));
		goto ErrExit;
	}

	filebuf = (char *)calloc(1, filestat.st_size);
	if (filebuf == NULL)
	{
		pub_log_error("[%s][%d] Calloc error, size=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, filestat.st_size, errno, strerror(errno));
		fclose(fp);
		goto ErrExit;
	}
	size = fread(filebuf, 1, filestat.st_size, fp);
	if (filestat.st_size != size)
	{
		pub_log_error("[%s][%d] fread error, size=[%d] read=[%d] errno=[%d]:[%s]",
			__FILE__, __LINE__, filestat.st_size, size, errno, strerror(errno));
		free(filebuf);
		fclose(fp);
		goto ErrExit;
	}
	fclose(fp);

	size = sizeof(sw_seqs_t) + (((sw_seqs_t *)filebuf)->head.max_seq - 1)*sizeof(sw_seqs_unit_t);
	if (size != filestat.st_size)
	{
		pub_log_error("%s, %d seqs_filesize != seqs_shmsize", __FILE__, __LINE__); 
		free(filebuf);
		goto ErrExit;
	}

	memcpy(shm_seqs, filebuf, size);
	free(filebuf);

	pub_log_info("seqs(size %d) restored from file", size);

	if (shm_seqs->head.status == 1)
	{
		for (i = 0; i< shm_seqs->head.bp_trc_cnt; i++)
		{
			shm_seqs->bp_trc_array[i].trn_base += TRACE_INC;
		}

		for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
		{
			shm_seqs->busi_trc_array[i].trn_base += TRACE_INC;
		}
	}

	return SW_OK;

ErrExit:
	for (i = 0; i< shm_seqs->head.bp_trc_cnt; i++)
	{
		shm_seqs->bp_trc_array[i].trn_base += 1000 * TRACE_INC;
	}

	for (i = 0; i < shm_seqs->head.busi_trc_cnt; i++)
	{
		shm_seqs->busi_trc_array[i].trn_base += 1000 * TRACE_INC;
	}
	return SW_ERROR;
}


sw_int_t seqs_recover()
{
	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	return (seqs_loc_recover(g_shm_seqs));
}

sw_int64_t seqs_get_bsn_trace(sw_char_t *bsn_name, sw_char_t *trace_no)
{
	int     idx = 0;
	sw_seqs_t*shm_seqs = g_shm_seqs;
	sw_seqs_unit_t  *unit = NULL;

	for (idx = 0; idx < shm_seqs->head.busi_trc_cnt; idx++)
	{
		if (strcmp(shm_seqs->busi_trc_array[idx].name, bsn_name) == 0)
		{
			break;
		}
	}
	if (idx == shm_seqs->head.busi_trc_cnt)
	{
		return 0;
	}

	unit = &shm_seqs->busi_trc_array[idx];
	sprintf(trace_no, "%s%0*lld", unit->trn_prefix, (int)(SW_TRACE_LEN - strlen(unit->trn_prefix)), unit->trn_base);

	return 0;
}

const char *seqs_get_syssts_desc()
{
	sw_int32_t      sys_status = 0;
	static char     desc[128];

	memset(desc, 0x0, sizeof(desc));
	sys_status = seqs_get_syssts();
	if (seqs_sys_is_run(sys_status))
	{
		strcpy(desc, "Running");
	}
	else if (seqs_sys_is_stop(sys_status))
	{
		strcpy(desc, "Shutdown");
	}
	else
	{
		strcpy(desc, "Unkown");
	}

	return desc;
}

int seqs_get_traceno_and_sysdate(sw_int64_t *trace_no, sw_char_t *sysdate)
{
	sw_int_t	len = 0;
	sw_int_t	ret = 0;
	sw_int64_t	tmptrace = 0;
	sw_char_t	*q = NULL;
	sw_char_t	*p = NULL;
	sw_char_t	buf[128];
	sw_char_t	trace[64];

	memset(buf, 0x0, sizeof(buf));
	memset(trace, 0x0, sizeof(trace));
	
	if (g_seqs_in_ares)
	{
		memset(buf, 0x0, sizeof(buf));
		ret = ares_comm(g_ares_fd, buf, 0, SEQS_BP_TRACE_DATE);
		if (ret < 0)
		{
			pub_log_error("[%s][%d] Ares get bp traceno error!", __FILE__, __LINE__);
			goto END;
		}
		p = strstr(buf, ARES_SEP);
		if (p == NULL)
		{
			pub_log_error("[%s][%d] Ares get bp traceno and sysdate error.", __FILE__, __LINE__);
			goto END;
		}

		len = p - buf;
		memset(trace, 0x0, sizeof(trace) - 1);
		strncpy(trace, buf, len);
		tmptrace = strtoll(trace, NULL, 10);

		q =  strstr(p + 1, ARES_SEP);
		if (q == NULL)
		{
			pub_log_error("[%s][%d] Ares get bp traceno and sysdate error.", __FILE__, __LINE__);
			goto END;
		}
		len = q - p - 1;
		strncpy(sysdate, p + 1, len);
		*trace_no = tmptrace;

		return SW_OK;
	}

END:

	if (g_shm_seqs == NULL)
	{
		pub_log_error("%s, %d, g_shm_seqs == NULL",__FILE__,__LINE__);
		return SW_ERROR;
	}

	ret = seqs_loc_new_trace_no(g_shm_seqs, NULL, trace);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get new trace no error!", __FILE__, __LINE__);
		alert_msg(ERR_TRCNO_FAILED, "get bp new trace no error.");
		return SW_ERROR;
	}

	tmptrace = strtoll(trace, NULL, 10);

	memset(buf, 0x00, sizeof(buf));
	ret = seqs_loc_get_sysdate(g_shm_seqs, buf, DATE_MAX_LEN);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] get new trace no error!", __FILE__, __LINE__);
		alert_msg(ERR_DATE_FAILED, "get bp date error.");
		return SW_ERROR;
	}
	strcpy(sysdate, buf);
	*trace_no = tmptrace;

	return SW_OK;
}

static sw_int_t seqs_print_flow(sw_char_t *buf)
{
	sw_int_t   i = 0;
	sw_int_t   cnt = 0;
	sw_int_t   no = 0;
	sw_int64_t base = 0;
	sw_int64_t current = 0;
	sw_char_t  name[64];
	sw_char_t  prefix[64];
	sw_char_t  tmp[255];
	sw_char_t  *pstr = buf;

	if (buf == NULL)
	{
		return SW_ERROR;
	}

	while (*pstr != '\0')
	{
		if (*pstr == '|')
		{
			cnt++;
			tmp[i] = '\0';
			i = 0;
			if (cnt == 1)
			{
				no = atoi(tmp);
			}
			else if (cnt == 2)
			{
				strncpy(name, tmp, 64);
			}
			else if (cnt == 3)
			{
				strncpy(prefix, tmp, 64);
			}
			else if (cnt == 4)
			{
				base = atoll(tmp);
			}
			else if (cnt == 5)
			{
				current = atoll(tmp);
				fprintf(stderr, "\t[%02d] %-32s %-12s %-12lld %-12lld\n",
						no, name, prefix, base, current);
			}
		}
		else if (*pstr == '@')
		{
			i = 0;
			cnt = 0;
			no = 0;
			base = 0;
			current = 0;
			memset(name, 0x0, sizeof(name));
			memset(prefix, 0x0, sizeof(prefix));
		}
		else if (*pstr == '~')
		{
			pstr++;
			break;
		}
		else
		{
			tmp[i++] = *pstr;
		}
		pstr++;
	}

	if (*pstr == '\0')
	{
		return SW_ERROR;
	}

	i = 0;
	while (*pstr != '~')
	{
		tmp[i++] = *pstr++;
	}
	tmp[i] = '\0';
	return atol(tmp);
}
