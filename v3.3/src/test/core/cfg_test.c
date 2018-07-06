#include "pub_cfg.h"
#include "cycle.h"
/*
cfg_get_sys(addr,cfg) 
cfg_get_slog(addr,log)
cfg_get_alog(addr,log)

sw_int_t cfg_set_path(sw_global_path_t *path);
sw_int_t cfg_size_calc(sw_syscfg_t *cfg);
sw_int_t cfg_set_offset(sw_cfgshm_hder_t *addr,sw_syscfg_t *cfg);
sw_int_t cfg_init(sw_cfgshm_hder_t *cfgaddr,sw_global_path_t  *path);
sw_int_t cfg_read_syscfg(sw_global_path_t *path, sw_syscfg_t *syscfg);
*/

int main(int argc, char *argv[])
{
	sw_int_t		ret = -1;
	char			*addr = NULL;
	sw_global_path_t	path;
	sw_syscfg_t		syscfg;
	sw_int32_t		i = 0;
	sw_int32_t		j = 0;
	sw_int32_t		count = 0;
	sw_logcfg_t		log;
	sw_prdt_cfg_t		*prdts = NULL;
	sw_chl_cfg_t		*channels = NULL;

	memset(&path,0x00,sizeof(sw_global_path_t));
	memset(&syscfg,0x00,sizeof(sw_syscfg_t));

	cfg_set_path(&path);

	pub_log_error("\n cfgpath[%s]\n chlfile[%s]\n data[%s]\n work[%s]\n sysfile[%s]\n prdt[%s]\n key[%s]\n job[%s]\n",
	path.cfgpath,path.chl_file,path.datapath,path.workpath,path.syscfg_file,path.prdt_file,
	path.key_file,path.job_file);

	ret = cfg_read_syscfg(&path,&syscfg);
	if (ret != SW_OK)
	{
		pub_log_error("cfg_read_syscfg error!");
		return -1;
	}
	
	pub_log_error("\n nm[%s]\nsid[%s]\n db[%d]\n job[%d]\n lsn[%d]\n proc[%d]\n runsize[%d]\n scan[%d]\n semsie[%d]\n seq[%d]\n ses[%d]\n share[%d]\n svr[%d]\n\n",
			syscfg.name,syscfg.sid,syscfg.db_max ,syscfg.job_max,syscfg.lsn_max,
			syscfg.processe_max,syscfg.runshmsize,syscfg.scantime,syscfg.semsize,syscfg.seq_max,
			syscfg.session_max,syscfg.share_pool_size,syscfg.svr_max);

	int len;
	len = cfg_size_calc(&syscfg);
	pub_log_error("[%d] cfg_size_calc len first",len);

	len = cfg_size_calc(&syscfg);
	pub_log_error("[%d] cfg_size_calc len  second",len);

	addr = malloc(len+1);
	memset(addr,0x00,len+1);
	sw_cfgshm_hder_t *ptr;
	ptr = (sw_cfgshm_hder_t *)addr;
	
	cfg_set_offset(ptr,&syscfg);

	pub_log_error("\n sys[%d]\n slg[%d]\n alg[%d]\n db[%d]\n seq[%d]\n chl[%d]\n prdt[%d]\n",
	ptr->sys_offset,ptr->slg_offset,ptr->alg_offset,ptr->db_offset,ptr->seq_offset,
	ptr->chl_offset,ptr->prdt_offset);
	
	ret = cfg_init(ptr,&path);
	if (ret != SW_OK)
	{
		pub_log_error("cfg_init error!");
		return -1;
	}
	pub_log_error("\n sys[%d]\n slg[%d]\n alg[%d]\n db[%d]\n seq[%d]\n chl[%d]\n prdt[%d]\n",
	ptr->sys_offset,ptr->slg_offset,ptr->alg_offset,ptr->db_offset,ptr->seq_offset,
	ptr->chl_offset,ptr->prdt_offset);
	
	memset(&syscfg,0x00,sizeof(sw_syscfg_t));
	cfg_get_sys(ptr,&syscfg) ;
	pub_log_error("nm[%s]\nsid[%s]\n db[%d]\n job[%d]\n lsn[%d]\n proc[%d]\n runsize[%d]\n scan[%d]\n semsie[%d]\n seq[%d]\n ses[%d]\n share[%d]\n svr[%d]\n",
	syscfg.name,syscfg.sid,syscfg.db_max ,syscfg.job_max,syscfg.lsn_max,
	syscfg.processe_max,syscfg.runshmsize,syscfg.scantime,syscfg.semsize,syscfg.seq_max,
	syscfg.session_max,syscfg.share_pool_size,syscfg.svr_max);

	pub_log_error("\n sys[%d]\n slg[%d]\n alg[%d]\n db[%d]\n seq[%d]\n chl[%d]\n prdt[%d]\n",
	ptr->sys_offset,ptr->slg_offset,ptr->alg_offset,ptr->db_offset,ptr->seq_offset,
	ptr->chl_offset,ptr->prdt_offset);

	prdts = (sw_char_t*)ptr + ptr->prdt_offset;
	count = ptr->prdt_use;

	/*print prdts info*/
	for (i = 0; i < count; i++)
	{
		pub_log_info("%s, %d, prdtname[%s] status[%d] shmid[%d]"
				, __FILE__, __LINE__, prdts[i].prdtname, prdts[i].status
				, prdts[i].shmid);
		for (j = 0; j < prdts[i].lsn_cnt; j++)
		{
			pub_log_info("%s, %d, lsnname[%d]=[%s]"
					, __FILE__, __LINE__, j, prdts[i].lsnname[j]);
		}
	}

	channels = (sw_char_t*)ptr + ptr->chl_offset;
	count = ptr->lsn_use;
	for (i = 0; i < count; i++)
	{
		pub_log_info("%s, %d, lsnname[%s] status[%d] shmid[%d]" 
				, __FILE__, __LINE__, channels[i].lsnname, channels[i].status
				,channels[i].shmid);
		for (j = 0; j < channels->prdt_cnt; j++)
		{
			pub_log_info("%s, %d, prdtname[%d]=[%s]"
					, __FILE__, __LINE__, j, channels[i].prdtname[j]);
		}
	}
	
	memset(&log,0x00,sizeof(sw_logcfg_t));
	cfg_get_slog(ptr,&log);
	
	pub_log_error("path[%s]\n name[%s]\n info[%s]\n size[%d]\n lvl[%d]\n mode[%d]\n",
	log.lgpath,log.lgname_fmt,log.lginfo_fmt,log.lgfile_size,log.lglvl,log.lgmode);
	memset(&log,0x00,sizeof(sw_logcfg_t));
	cfg_get_alog(ptr,&log);

	pub_log_error("path[%s]\n name[%s]\n info[%s]\n size[%d]\n lvl[%d]\n mode[%d]\n",
	log.lgpath,log.lgname_fmt,log.lginfo_fmt,log.lgfile_size,log.lglvl,log.lgmode);


	free(addr);
}

