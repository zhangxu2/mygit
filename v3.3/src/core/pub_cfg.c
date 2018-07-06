/*********************************************************************
 *** version : V3.0
 *** author  : zhang hailu
 *** create  : 2013-5-19
 *** module  : core
 *** name    : pub_cfg.c
 *** function:  
 *** lists   :  
 *** notice  :  
 *** modified:  
 ***   author:  
 ***   date  :  
 ***  content:  
 ********************************************************************/

#include "pub_cfg.h"
#include "pub_xml.h"
#include "pub_buf.h"
#include "pub_ares.h"
#include "pub_string.h"
#include "des3.h"

typedef union { void* p; int i; long l; double d; void (*f)(); }  sw_word_t;
#define ALIGNSIZE(x, size) pub_mem_align(x,size)
#define ALIGNWORD(x) ALIGNSIZE(x, sizeof(sw_word_t))

static sw_int_t read_db_cell(sw_xmlnode_t *node,sw_dbcfg_t *dbcfg);
static sw_int_t read_prdt_cell(sw_xmlnode_t *node,sw_prdt_cfg_t *prdtcfg);
static sw_int_t read_chl_cell(sw_xmlnode_t *node,sw_chl_cfg_t *chlcfg);
static sw_int_t read_seqn_cell(sw_xmlnode_t *node,sw_seqcfg_t *seqcfg);
static sw_int_t read_log_cell(sw_xmlnode_t *node,sw_logcfg_t *logcfg);
static sw_int_t read_safd_cell(sw_xmlnode_t *node,sw_safd_cfg_t *safdcfg);
static sw_int_t read_syscfg(sw_xmltree_t *xml_tree,sw_syscfg_t *syscfg);
static sw_int_t read_prdt_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder);
static sw_int_t read_chl_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder);
static sw_int_t read_db_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder);
static sw_int_t read_alog_cfg(sw_xmlnode_t *pnode, alog_cfg_t *alog);
static sw_int_t read_ares_cfg(sw_xmlnode_t *pnode, ares_cfg_t *ares);
static int print_db_info(sw_cfgshm_hder_t *shmaddr);

extern int readfile(const char *filename, sw_buf_t *readbuf);
sw_int_t cfg_get_alog_cfg(sw_cfgshm_hder_t*addr, alog_cfg_t *alog);
sw_int_t cfg_get_ares_cfg(sw_cfgshm_hder_t*addr, ares_cfg_t *ares);

sw_int_t cfg_set_path(sw_global_path_t *path)
{
	if (path == NULL )
	{
		return -1;
	}

	char	*ptr = NULL;

	ptr = getenv("SWWORK");
	if ( ptr == NULL || strlen(ptr) == 0 || strlen(ptr) >= PATH_LEN)
	{
		pub_log_stderr("Environment variables SWWORK not set!\n");
		return -1;
	}

	strncpy(path->workpath,ptr,PATH_LEN); 
	snprintf(path->cfgpath,PATH_LEN,"%s/%s",path->workpath,SW_CFG_PATH);	
	snprintf(path->key_file,PATH_LEN,"%s/%s",path->cfgpath,SW_KEY_FILE);
	snprintf(path->syscfg_file,PATH_LEN,"%s/%s",path->cfgpath,SW_SYSCFG_FILE);
	snprintf(path->job_file,PATH_LEN,"%s/%s",path->cfgpath,SW_JOB_FILE);
	snprintf(path->chl_file,PATH_LEN,"%s/%s",path->cfgpath,SW_CHANNEL_FILE);
	snprintf(path->prdt_file,PATH_LEN,"%s/%s",path->cfgpath,SW_PRDT_FILE);
	snprintf(path->dbcfg_file,PATH_LEN,"%s/%s",path->cfgpath,SW_DBCFG_FILE);
	snprintf(path->datapath,PATH_LEN,"%s/%s",path->workpath,SW_MSG_FILE);

	return SW_OK;
}


sw_int_t cfg_size_calc(const sw_syscfg_t *cfg)
{
	sw_int_t len;
	len = 0;
	/* calc the off_set and Determine whether the memory is sufficient */
	len += ALIGNWORD(sizeof(sw_cfgshm_hder_t));
	len += ALIGNWORD(sizeof(sw_syscfg_t));
	len += ALIGNWORD(sizeof(sw_logcfg_t));
	len += ALIGNWORD(sizeof(sw_logcfg_t));
	len += ALIGNWORD(sizeof(alog_cfg_t));
	len += ALIGNWORD(sizeof(ares_cfg_t));
	len += ALIGNWORD(sizeof(sw_safd_cfg_t));	
	len += ALIGNWORD(sizeof(sw_dbcfg_t)*cfg->db_max);
	len += ALIGNWORD(sizeof(sw_seqcfg_t)*cfg->seq_max);
	len += ALIGNWORD(sizeof(sw_chl_cfg_t)*cfg->lsn_max);	
	len += ALIGNWORD(sizeof(sw_prdt_cfg_t)*cfg->prdt_max);

	return len;
}

sw_int_t cfg_set_offset(sw_cfgshm_hder_t *addr,sw_syscfg_t *cfg)
{
	if (addr == NULL )
	{
		pub_log_error("[%s][%d] cfg_set_offset input addr=[%p] illegal!",__FILE__,__LINE__,addr);
		return SW_ERR;
	}
	/* set the off_set */
	addr->sys_offset = ALIGNWORD(sizeof(sw_cfgshm_hder_t));
	addr->slg_offset = addr->sys_offset + ALIGNWORD(sizeof(sw_syscfg_t));
	addr->alg_offset = addr->slg_offset + ALIGNWORD(sizeof(sw_logcfg_t));
	addr->alog_offset = addr->alg_offset + ALIGNWORD(sizeof(sw_logcfg_t));	
	addr->ares_offset = addr->alog_offset + ALIGNWORD(sizeof(alog_cfg_t));	
	addr->safd_offset = addr->ares_offset + ALIGNWORD(sizeof(ares_cfg_t));
	addr->db_offset = addr->safd_offset + ALIGNWORD(sizeof(sw_safd_cfg_t));
	addr->seq_offset = addr->db_offset + ALIGNWORD(sizeof(sw_dbcfg_t)*cfg->db_max);
	addr->chl_offset = addr->seq_offset + ALIGNWORD(sizeof(sw_seqcfg_t)*cfg->seq_max);
	addr->prdt_offset = addr->chl_offset + ALIGNWORD(sizeof(sw_chl_cfg_t)*cfg->lsn_max);	

	return SW_OK;
}

/******************************************************************************
 *** function  : cfg_init
 *** author    : zhang hailu
 *** create    : 2013-5-22 17:25
 *** call lists:  
 *** inputs    : 
 ***     arg1  :  
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_int_t cfg_init(sw_cfgshm_hder_t *cfgaddr,sw_global_path_t  *path)
{
	int	i = 0;
	int	ret = 0;
	int	workmode = 0;
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_syscfg_t	*syscfg = NULL;	
	sw_logcfg_t	*log = NULL;
	sw_seqcfg_t	*seq = NULL;
	sw_safd_cfg_t	*saf = NULL;
	alog_cfg_t	*alog = NULL;
	ares_cfg_t	*ares = NULL;

	xml = pub_xml_crtree(path->syscfg_file);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] pub_xml_crtree(%s) error! ",__FILE__,__LINE__,path->syscfg_file);
		return SW_ERR;
	}

	node = pub_xml_locnode(xml, ".SWCFG.GLOBAL.WORKMODE");
	if (node != NULL && node->value != NULL && strcasecmp(node->value, "MP") == 0)
	{
		workmode = 1;
	}

	node = pub_xml_locnode(xml, ".SWCFG.ALOG");
	if (node != NULL)
	{
		alog = (alog_cfg_t *)BLOCKAT(cfgaddr, cfgaddr->alog_offset);
		if (read_alog_cfg(node, alog) != SW_OK)
		{
			pub_log_error("[%s][%d] Read alog cfg error!", __FILE__, __LINE__);
			return SW_ERR;
		}
		pub_log_info("[%s][%d] Read alog info success! use=[%d] ip=[%s] port=[%d]",
				__FILE__, __LINE__, alog->use, alog->ip, alog->port);
	}

	if (workmode)
	{
		node = pub_xml_locnode(xml, ".SWCFG.ARES");
		if (node != NULL)
		{
			ares = (ares_cfg_t *)BLOCKAT(cfgaddr, cfgaddr->ares_offset);
			if (read_ares_cfg(node, ares) != SW_OK)
			{
				pub_log_error("[%s][%d] Read ares cfg error!", __FILE__, __LINE__);
				return SW_ERR;
			}
			pub_log_info("[%s][%d] Read ares info success! use=[%d] ip=[%s] port=[%d]",
					__FILE__, __LINE__, ares->use, ares->ip, ares->port);
		}
	}

	syscfg = (sw_syscfg_t *)BLOCKAT(cfgaddr,cfgaddr->sys_offset);	
	ret = read_syscfg(xml,syscfg);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] read_sys_cfg error ",__FILE__,__LINE__);
		goto EXIT_ERR;
	}	

	node=pub_xml_locnode(xml,".SWCFG.SYSLOG");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] SYSLOG isn't exstence",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	log = (sw_logcfg_t *)BLOCKAT(cfgaddr,cfgaddr->slg_offset);
	ret = read_log_cell(node,log);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read syslog cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	node=pub_xml_locnode(xml,".SWCFG.APPLOG");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] APLOG isn't exstence",__FILE__,__LINE__);
		goto EXIT_ERR;
	}
	log = (sw_logcfg_t *)BLOCKAT(cfgaddr,cfgaddr->alg_offset);
	ret = read_log_cell(node,log);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read applog cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	node=pub_xml_locnode(xml,".SWCFG.SAFD");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] SAFD isn't exstence",__FILE__,__LINE__);
		goto EXIT_ERR;
	}
	saf  = (sw_safd_cfg_t *)BLOCKAT(cfgaddr,cfgaddr->safd_offset);
	ret = read_safd_cell(node,saf);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read safd cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	node=pub_xml_locnode(xml,".SWCFG.SEQN");
	if (node == NULL)
	{
		pub_log_error("[%s][%d] SEQN isn't exstence",__FILE__,__LINE__);
		goto EXIT_ERR;
	}
	i = 0;
	seq = (sw_seqcfg_t *)BLOCKAT(cfgaddr,cfgaddr->seq_offset);
	do
	{
		ret = read_seqn_cell(node, seq);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] read SEQN cfg error!",__FILE__,__LINE__);
			goto EXIT_ERR;
		}
		node = node->next;
		i++;
		seq++;
	}while(node &&i < syscfg->seq_max);
	cfgaddr->seq_use = i;

	ret = read_db_cfg(path, cfgaddr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read DATABASE cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	pub_xml_deltree(xml);
	print_db_info(cfgaddr);

	ret = read_prdt_cfg(path,cfgaddr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read products.xml cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	ret = read_chl_cfg(path,cfgaddr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read channels.xml cfg error!",__FILE__,__LINE__);
		goto EXIT_ERR;
	}

	return  SW_OK;

EXIT_ERR:
	if (xml != NULL)
	{
		pub_xml_deltree(xml);
	}

	return SW_ERR;

}

sw_int_t cfg_get_sys(const sw_cfgshm_hder_t *addr,sw_syscfg_t *cfg)
{
	pub_mem_memcpy(cfg,BLOCKAT(addr,addr->sys_offset),sizeof(sw_syscfg_t));
	return SW_OK;
}

sw_int_t cfg_get_slog(sw_cfgshm_hder_t *addr,sw_logcfg_t*log)
{
	pub_mem_memcpy(log,BLOCKAT(addr,addr->slg_offset),sizeof(sw_logcfg_t));
	return SW_OK;
}

sw_int_t cfg_get_alog(sw_cfgshm_hder_t*addr,sw_logcfg_t*log)
{
	pub_mem_memcpy(log,BLOCKAT(addr,addr->alg_offset),sizeof(sw_logcfg_t));
	return SW_OK;
}
sw_int_t cfg_get_saf(sw_cfgshm_hder_t*addr,sw_safd_cfg_t *safcfg)
{
	pub_mem_memcpy(safcfg,BLOCKAT(addr,addr->safd_offset),sizeof(sw_safd_cfg_t));
	return SW_OK;
}

sw_int_t cfg_get_prdt_info(sw_prdt_cfg_t * prdt,sw_cfgshm_hder_t *addr,int id)
{
	if (prdt == NULL || id < 0 ||  addr == NULL)
	{
		pub_log_error("[%s][%d]cfg_get_prdt_shmid input argv is illegal",__FILE__,__LINE__);
		return SW_ERR;
	}

	sw_prdt_cfg_t *ptr;

	ptr = (sw_prdt_cfg_t *) BLOCKAT(addr,addr->prdt_offset);

	if ( id < addr->prdt_use)
	{
		pub_mem_memcpy(prdt,ptr+id,sizeof(sw_prdt_cfg_t));
		return SW_OK;
	}
	else
	{
		pub_log_error("[%s][%d] ",__FILE__,__LINE__);
		return SW_ERR;
	}

}

sw_int_t cfg_get_prdt_shmid(sw_char_t * prdt_name,sw_cfgshm_hder_t *addr)
{
	if (prdt_name == NULL || prdt_name[0] == '\0' ||  addr == NULL)
	{
		pub_log_error("[%s][%d]cfg_get_prdt_shmid input prdt_name is null",__FILE__,__LINE__);
		return SW_ERR;
	}

	sw_int32_t shmid;
	sw_int32_t i;
	sw_prdt_cfg_t * prdt;

	prdt = (sw_prdt_cfg_t *) BLOCKAT(addr,addr->prdt_offset);

	shmid = -1;
	for (i = 0; i < addr->prdt_use ;i ++)
	{
		if ( strcmp(prdt[i].prdtname,prdt_name) == 0)
		{
			shmid = prdt[i].shmid ;
			break;
		}
	}
	return shmid;
}

/* set prdt shmid */
sw_int_t cfg_set_prdt_shmid(sw_int32_t shmid,sw_char_t * prdt_name,sw_cfgshm_hder_t *addr)
{
	if (prdt_name == NULL || prdt_name[0] == '\0' || shmid <= 0 || addr == NULL)
	{
		pub_log_error("[%s][%d]cfg_get_prdt_shmid input prdt_name is null",__FILE__,__LINE__);
		return SW_ERR;
	}

	sw_int32_t i;
	sw_prdt_cfg_t * prdt;

	prdt = (sw_prdt_cfg_t *) BLOCKAT(addr,addr->prdt_offset);

	shmid = -1;
	for (i = 0; i < addr->prdt_use ;i ++)
	{
		if ( strcmp(prdt[i].prdtname,prdt_name) == 0)
		{
			prdt[i].shmid = shmid;
			return SW_OK;;
		}
	}
	return SW_ERR;
}

sw_int_t cfg_get_chl_shmid(sw_char_t * chl_name,sw_cfgshm_hder_t *addr)
{
	if (chl_name == NULL || chl_name[0] == '\0' || addr == NULL)
	{
		pub_log_error("[%s][%d]cfg_get_chl_shmid input prdt_name or addr is null",__FILE__,__LINE__);
		return SW_ERR;
	}

	sw_int32_t shmid;
	sw_int32_t i;
	sw_chl_cfg_t * chl;

	chl = (sw_chl_cfg_t *) BLOCKAT(addr,addr->chl_offset);

	shmid = -1;
	for (i = 0; i < addr->prdt_use ;i ++)
	{
		if ( strcmp(chl[i].lsnname,chl_name) == 0)
		{
			shmid = chl[i].shmid ;
			break;
		}
	}
	return shmid;
}

sw_int_t cfg_set_chl_shmid(sw_int32_t shmid,sw_char_t * chl_name,sw_cfgshm_hder_t *addr)
{
	if (shmid <= 0|| chl_name == NULL || chl_name[0] == '\0' || addr == NULL)
	{
		pub_log_error("[%s][%d]cfg_get_chl_shmid input prdt_name or addr is null",__FILE__,__LINE__);
		return SW_ERR;
	}

	sw_int32_t i;
	sw_chl_cfg_t * chl;

	chl = (sw_chl_cfg_t *) BLOCKAT(addr,addr->chl_offset);

	shmid = -1;
	for (i = 0; i < addr->prdt_use ;i ++)
	{
		if ( strcmp(chl[i].lsnname,chl_name) == 0)
		{
			chl[i].shmid = shmid;
			return SW_OK;
		}
	}
	return SW_ERR;
}

sw_int_t cfg_read_syscfg(const sw_global_path_t *path, sw_syscfg_t *syscfg)
{
	int	ret = 0;
	sw_xmltree_t *xml = NULL;

	xml = pub_xml_crtree((char *)path->syscfg_file);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] pub_xml_crtree(%s) error! ",__FILE__,__LINE__,path->syscfg_file);
		return SW_ERR;
	}

	ret = read_syscfg(xml,syscfg);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] read_sys_cfg error ",__FILE__,__LINE__);
		pub_xml_deltree(xml);
		return SW_ERR;
	}
	pub_xml_deltree(xml);
	return SW_OK;
}

/******************************************************************************
 *** function  : read_syscfg
 *** call lists:  
 *** inputs    : 
 ***     arg1  :  
 *** outputs   :  
 ***     arg1  : 
 *** return    :  0:success  -1:fail
 *** notice    : 
 *** author    : # zhang hailu # 2013-5-19 11:17
 *** Moditfy   : # Qifeng.zou # 2013.09.10 #
 ******************************************************************************/
static sw_int_t read_syscfg(sw_xmltree_t *xml_tree,sw_syscfg_t *syscfg)
{
	sw_xmlnode_t *curr_node = NULL;
	sw_xmlnode_t *node1 = NULL;

	assert(xml_tree!=NULL );
	assert( syscfg!=NULL) ;
	pub_mem_memset((char*)syscfg, 0x00, sizeof(sw_syscfg_t));

	/*set default syscfg param*/
	syscfg->lsn_max = LSN_MAX_DEFAULT;
	syscfg->svr_max = SVR_MAX_DEFAULT;
	syscfg->prdt_max = PRDT_MAX_DEFAULT;
	syscfg->processe_max = PRO_MAX_DEFAULT;
	syscfg->session_max = SESSION_MAX_DEFAULT;
	syscfg->job_max = JOB_MAX_DEFAULT;
	syscfg->db_max = DB_MAX_DEFAULT;
	syscfg->scantime = SCANTIME_DEFAULT;
	syscfg->share_pool_size = SHAREPOOL_DEFAULT;
	syscfg->semsize = SEMSIZE;
	syscfg->run_shmid = SW_INVALID_IPC_ID;
	syscfg->vars_shmid = SW_INVALID_IPC_ID;
	syscfg->seq_max = SEQS_DEFAULT;

	curr_node=pub_xml_locnode(xml_tree,".SWCFG.GLOBAL");
	if(curr_node==NULL)
	{
		return SW_ERR;
	}

	node1 = curr_node->firstchild;
	while(node1!=NULL)
	{
		if(0 == strcmp(node1->name,"ESWSID"))
		{
			if( node1->value==NULL 
					|| strlen(node1->value)==0 )
			{
				pub_log_error("[%s][%d]the ESWSID is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				pub_mem_memset(syscfg->sid,0x00,sizeof(syscfg->sid));
				strncpy(syscfg->sid,node1->value,SID_LEN);
			}
		}
		else if(0 == strcmp(node1->name,"ESWNAME"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{
				pub_log_error("[%s][%d]the ESWNAME  is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				pub_mem_memset(syscfg->name,0x00,sizeof(syscfg->name));
				strncpy(syscfg->name,node1->value,NAME_LEN);
			}
		}
		else if(0 == strcmp(node1->name,"LSNMAX"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->lsn_max= LSN_MAX_DEFAULT;
				pub_log_info("[%s][%d]the LSNMAX is empty,use default value[%d]",__FILE__, __LINE__,syscfg->lsn_max);
			}
			else
			{
				syscfg->lsn_max = (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"SVRMAX"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->svr_max= SVR_MAX_DEFAULT;
				pub_log_info("[%s][%d]the SVRMAX is empty,use default value[%d]",__FILE__, __LINE__,syscfg->svr_max);
			}
			else
			{
				syscfg->svr_max = (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name, "PRDTMAX"))
		{
			if (node1->value == NULL || strlen(node1->value) == 0)
			{
				syscfg->prdt_max = PRDT_MAX_DEFAULT;
				pub_log_info("%s, %d, the PRDTMAX is empty, use default value[%d]"
						, __FILE__, __LINE__, syscfg->prdt_max);
			}
			else
			{
				syscfg->prdt_max = (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"PROCESS"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->processe_max= PRO_MAX_DEFAULT;
				pub_log_info("[%s][%d]the PROCESS  is empty,use default value[%d]",__FILE__, __LINE__,syscfg->processe_max);
			}
			else
			{
				syscfg->processe_max= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"SESSION"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->session_max= SESSION_MAX_DEFAULT;
				pub_log_info("[%s][%d]the SESSION is empty,use default value[%d]",__FILE__, __LINE__,syscfg->session_max);
			}
			else
			{
				syscfg->session_max= (int )strtol(node1->value,NULL,0);
			}
		}
		else  if(0 == strcmp(node1->name,"SEQMAX"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->seq_max = SEQS_DEFAULT;
				pub_log_info("[%s][%d]the SEQMAX is empty,use default value[%d]",__FILE__, __LINE__, syscfg->seq_max);
			}
			else
			{
				syscfg->seq_max= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"JOBMAX"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->job_max= JOB_MAX_DEFAULT;
				pub_log_info("[%s][%d]the JOBMAX is empty,use default value[%d]",__FILE__, __LINE__, syscfg->job_max);
			}
			else
			{
				syscfg->job_max= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"DBMAX"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->db_max= DB_MAX_DEFAULT;
				pub_log_info("[%s][%d]the DBMAX is empty,use default value[%d]",__FILE__, __LINE__, syscfg->db_max);
			}
			else
			{
				syscfg->db_max= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"SCANTIME"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->scantime= SCANTIME_DEFAULT;
				pub_log_info("[%s][%d]the SCANTIME is empty,use default value[%d]",__FILE__, __LINE__, syscfg->scantime);
			}
			else
			{
				syscfg->scantime= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"SHAREPOOLSIZE"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{			
				syscfg->share_pool_size= SHAREPOOL_DEFAULT;
				pub_log_info("[%s][%d]the SHAREPOOLSIZE is empty,use default value[%d]",__FILE__, __LINE__, syscfg->share_pool_size);
			}
			else
			{
				syscfg->share_pool_size = (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"RUNSHMSIZE"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{
				syscfg->runshmsize= RUNSHMSIZE;
				pub_log_info("[%s][%d]the RUNSHMSIZE is empty,use default value[%d]",__FILE__,__LINE__, syscfg->runshmsize);
			}
			else
			{
				syscfg->runshmsize= (int )strtol(node1->value,NULL,0);
			}
		}
		else if(0 == strcmp(node1->name,"SEMSIZE"))
		{
			if( node1->value==NULL || strlen(node1->value)==0 )
			{
				syscfg->semsize = SEMSIZE;
				pub_log_info("[%s][%d]The SEMSIZE is empty,use default value[%d]",__FILE__, __LINE__, syscfg->semsize);
			}
			else
			{
				syscfg->semsize= (int )strtol(node1->value,NULL,0);
			}
		}

		node1=node1->next;
	}

	return SW_OK;
}

static sw_int_t read_db_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder)
{
	sw_int32_t i;
	sw_int_t ret;
	sw_xmltree_t *xml;
	sw_xmlnode_t *node;
	sw_syscfg_t  *syscfg;
	sw_dbcfg_t *db;

	if (path == NULL || cfghder == NULL)
	{
		pub_log_error("[%s][%d] read_lsn_cfg input argv error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	if (access(path->dbcfg_file, F_OK) < 0)
	{
		cfghder->db_use = 0;
		pub_log_info("[%s][%d] 数据库配置不存在!", __FILE__, __LINE__);
		return SW_OK;
	}

	xml = cfg_read_xml(path->dbcfg_file);
	if (xml == NULL )
	{
		pub_log_error("[%s][%d] pub_xml_crtree [%s] error!",__FILE__,__LINE__,path->dbcfg_file);
		return SW_ERR;
	}


	node = pub_xml_locnode(xml,".DBCONFIG.DATABASE");
	if (node == NULL )
	{
		pub_log_error("[%s][%d]  [%s] file must have DBCONFIG.DATABASE !",__FILE__,__LINE__,path->dbcfg_file);
		goto ERR_EXIT;
	}

	syscfg = (sw_syscfg_t *) BLOCKAT(cfghder,cfghder->sys_offset);

	i = 0;
	pub_log_debug("[%s][%d] db_offset=[%d]", __FILE__, __LINE__, cfghder->db_offset);
	db = (sw_dbcfg_t *)BLOCKAT(cfghder,cfghder->db_offset);
	do
	{
		ret = read_db_cell(node, db);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] read DATABASE cfg error!",__FILE__,__LINE__);
			goto ERR_EXIT;
		}
		if (strcasecmp(db->dbtype, "ORACLE") != 0)
		{
			db->exptime = 0;
		}
		node = node->next;
		i++;
		db++;
	}while(node &&i < syscfg->db_max);
	cfghder->db_use = i;
	print_db_info(cfghder);

	pub_xml_deltree(xml);
	return SW_OK;

ERR_EXIT:

	pub_xml_deltree(xml);
	return SW_ERR;
}

static sw_int_t read_db_cell(sw_xmlnode_t *node,sw_dbcfg_t *dbcfg)
{
	sw_xmlnode_t *curr_node;
	int  db_enc = 0;

	char output[100];

	assert(node!= NULL );
	if (getenv("DB_ENC") != NULL && strcasecmp(getenv("DB_ENC"), "true") == 0)
	{
		db_enc = 1;
	}
	curr_node = node->firstchild;

	while(curr_node != NULL)
	{
		if(strcmp(curr_node->name,"DBID")==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]DBID is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				pub_mem_memset(dbcfg->dbid,0x00,sizeof(dbcfg->dbid));
				strncpy(dbcfg->dbid,curr_node->value,sizeof(dbcfg->dbid)-1);
			}
		}
		else  if(strcmp(curr_node->name,"DBNAME")==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]DBNAME is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				pub_mem_memset(dbcfg->dbname,0x00,sizeof(dbcfg->dbname));
				strncpy(dbcfg->dbname,curr_node->value,NAME_LEN);
			}
		}
		/* delete by zhanghailu 20130-9-3
		   else if(strcmp(curr_node->name,"DBMODULE")==0)
		   {
		   if(curr_node->value == NULL||strlen(curr_node->value) == 0)
		   {
		   pub_log_error(" DBMODULE is empty");
		   return SW_ERR;
		   }
		   else
		   {
		   pub_mem_memset(dbcfg->dbmoule,0x00,sizeof(dbcfg->dbmoule));
		   strncpy(dbcfg->dbmoule,curr_node->value,NAME_LEN);
		   }
		   }
		 */
		else if(strcmp(curr_node->name,"DBSID")==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]DBMODULE is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				pub_mem_memset(dbcfg->dbsid,0x00,sizeof(dbcfg->dbsid));
				strncpy(dbcfg->dbsid,curr_node->value,NAME_LEN);
			}

		}
		else if(strcmp(curr_node->name,"DBUSER")==0 )
		{	
			pub_mem_memset(dbcfg->dbuser,0x00,sizeof(dbcfg->dbuser));
			if (curr_node->value != NULL&&strlen(curr_node->value) != 0)
			{
				strncpy(dbcfg->dbuser,curr_node->value,sizeof(dbcfg->dbuser));
			}
		}
		else if(strcmp(curr_node->name,"DBPASSWD")==0)
		{	
			pub_mem_memset(dbcfg->dbpasswd,0x00,sizeof(dbcfg->dbpasswd));
			if (curr_node->value != NULL&&strlen(curr_node->value) != 0)
			{
				if (db_enc)
				{
					memset(output,0x00,sizeof(output));
					pub_des3_dec(curr_node->value, output, strlen(curr_node->value), NULL);
					strncpy(dbcfg->dbpasswd,output,sizeof(dbcfg->dbpasswd)-1);
				}
				else
				{
					strncpy(dbcfg->dbpasswd,curr_node->value,sizeof(dbcfg->dbpasswd)-1);	
				}
			}
		}
		else if(strcmp(curr_node->name,"DBTYPE")==0)
		{	
			pub_mem_memset(dbcfg->dbtype,0x00,sizeof(dbcfg->dbtype));
			if (curr_node->value != NULL&&strlen(curr_node->value) != 0)
			{
				strncpy(dbcfg->dbtype,curr_node->value,sizeof(dbcfg->dbtype)-1);	
			}
		}
		else if (strcmp(curr_node->name, "EXPTIME") == 0)
		{
			if (curr_node->value != NULL && curr_node->value[0] != '\0')
			{
				dbcfg->exptime = atoi(curr_node->value);
			}
		}
		/* delete by zhanghailu 2013-9-3
		   else if(strcmp(curr_node->name,"DBIDTF")==0 )
		   {
		   if(curr_node->value == NULL||strlen(curr_node->value) == 0)
		   {
		   pub_log_error(" DBIDTF is empty");
		   return SW_ERR;
		   }
		   else
		   {

		   if (curr_node->value[0] == '0')
		   {
		   dbcfg->dbidtf = DB_IDTF_LOCAL;
		   }
		   else
		   {
		   dbcfg->dbidtf = DB_IDTF_USER;
		   }
		   }			
		   }
		 */
		curr_node = curr_node->next;
	}

	return 0;
}
static sw_int_t read_log_cell(sw_xmlnode_t *node,sw_logcfg_t *logcfg)
{
	sw_xmlnode_t *curr_node;

	assert(node!= NULL);
	assert(logcfg != NULL );
	logcfg->lgfile_size= LOG_FILE_SIZE_DEF;

	curr_node = node->firstchild;
	while(curr_node != NULL)
	{
		if(pub_mem_memcmp(curr_node->name,"LOGMODE",7)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				logcfg->lgmode = LOG_FILE;
			}
			else
			{
				logcfg->lgmode= atoi(curr_node->value);
			}
		}

		else  if(pub_mem_memcmp(curr_node->name,"LOGLVL",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				logcfg->lglvl = SW_LOG_INFO;
			}
			else
			{
				logcfg->lglvl= atoi(curr_node->value);
			}
			pub_log_debug("[%s][%d] LOGLEVEL=[%d]", __FILE__, __LINE__, logcfg->lglvl);
		}
		else if(pub_mem_memcmp(curr_node->name,"LOGPATH",7)==0)
		{
			pub_mem_memset(logcfg->lgpath,0x00,sizeof(logcfg->lgpath));
			if(curr_node->value != NULL && strlen(curr_node->value) > 0)
			{						
				strncpy(logcfg->lgpath,curr_node->value,sizeof(logcfg->lgpath)-1);
			}
		}
		else if(pub_mem_memcmp(curr_node->name,"LOGINFOFMT",10)==0)
		{
			pub_mem_memset(logcfg->lginfo_fmt,0x00,sizeof(logcfg->lginfo_fmt));
			if(curr_node->value != NULL && strlen(curr_node->value) > 0)
			{						
				strncpy(logcfg->lginfo_fmt,curr_node->value,sizeof(logcfg->lginfo_fmt)-1);
			}

		}
		else if(pub_mem_memcmp(curr_node->name,"LOGNAMEFMT",10)==0 )
		{			
			pub_mem_memset(logcfg->lgname_fmt,0x00,sizeof(logcfg->lgname_fmt));
			if(curr_node->value != NULL && strlen(curr_node->value) > 0)
			{						
				strncpy(logcfg->lgname_fmt,curr_node->value,sizeof(logcfg->lgname_fmt)-1);
			}			
		}
		else if(pub_mem_memcmp(curr_node->name,"LOGFILESIZE",11)==0 )
		{	
			if(curr_node->value != NULL && strlen(curr_node->value) != 0)
			{
				logcfg->lgfile_size= pub_cfg_parse_size(curr_node->value);
			}		
		}		
		curr_node = curr_node->next;
	}

	return SW_OK;

}

static sw_int_t read_safd_cell(sw_xmlnode_t *node,sw_safd_cfg_t *safdcfg)
{
	sw_xmlnode_t *curr_node;

	assert(node!= NULL);
	assert(safdcfg != NULL );

	curr_node = node->firstchild;
	while(curr_node != NULL)
	{
		if(pub_mem_memcmp(curr_node->name,"SAFUSE",6)==0)
		{
			if(curr_node->value == NULL
					||strlen(curr_node->value) == 0
					||curr_node->value[0] == '0')  
			{
				safdcfg->status = SAFD_NOUSE;
			}
			else
			{
				safdcfg->status = SAFD_USE;
			}
		}

		else  if(pub_mem_memcmp(curr_node->name,"DBCFG",5)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				safdcfg->dbcfg[0] = '\0';
			}
			else
			{
				strncpy(safdcfg->dbcfg,curr_node->value,sizeof(safdcfg->dbcfg)-1);
			}
		}		
		curr_node = curr_node->next;
	}

	return SW_OK;

}

static sw_int_t read_seqn_cell(sw_xmlnode_t *node,sw_seqcfg_t *seqcfg)
{
	sw_xmlnode_t *curr_node;

	assert(node!= NULL );
	assert(seqcfg != NULL );

	curr_node = node->firstchild;

	while(curr_node != NULL)
	{
		if(pub_mem_memcmp(curr_node->name,"SQID",4)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]SQID is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				strncpy(seqcfg->sqid,curr_node->value,sizeof(seqcfg->sqid)-1);
			}
		}

		else  if(pub_mem_memcmp(curr_node->name,"SQNAME",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]SQNAME is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				strncpy(seqcfg->sqname,curr_node->value,sizeof(seqcfg->sqname)-1);
			}
		}
		else if(pub_mem_memcmp(curr_node->name,"SQMODE",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				seqcfg->sqmode = SEQ_SHM;
			}
			else
			{
				seqcfg->sqmode = atoi(curr_node->value);
			}
		}
		else if(pub_mem_memcmp(curr_node->name,"SQSTART",7)==0)
		{
			pub_mem_memset(seqcfg->sqstart,0x00,sizeof(seqcfg->sqstart));
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				seqcfg->sqstart[0] = '0';
			}
			else
			{
				strncpy(seqcfg->sqstart,curr_node->value,sizeof(seqcfg->sqstart)-1);
			}

		}
		else if(pub_mem_memcmp(curr_node->name,"SQSTEP",7)==0 )
		{			
			pub_mem_memset(seqcfg->sqstep,0x00,sizeof(seqcfg->sqstep));
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				seqcfg->sqstep[0] = '0';
			}
			else
			{
				strncpy(seqcfg->sqstep,curr_node->value,sizeof(seqcfg->sqstep)-1);
			}			
		}
		else if(pub_mem_memcmp(curr_node->name,"SQMAX",5)==0 )
		{	
			pub_mem_memset(seqcfg->sqmax,0x00,sizeof(seqcfg->sqmax));
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				seqcfg->sqmax[0] = '0';
			}
			else
			{
				strncpy(seqcfg->sqmax,curr_node->value,sizeof(seqcfg->sqmax)-1);
			}		
		}
		else if(pub_mem_memcmp(curr_node->name,"SQCYCLE",7)==0 )
		{	

			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				seqcfg->sqcycle = SEQ_CYC;
			}
			else
			{
				seqcfg->sqcycle = atoi(curr_node->value);
			}		
		}
		else if(pub_mem_memcmp(curr_node->name,"SQPREFIX",8)==0 )
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]SQPREFIX is empty", __FILE__, __LINE__);
				return SW_ERROR;
			}
			else
			{
				strncpy(seqcfg->sqprefix,curr_node->value,sizeof(seqcfg->sqprefix)-1);
			}		
		}

		curr_node = curr_node->next;
	}

	return 0;

}

static sw_int_t read_prdt_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder)
{
	sw_int_t ret;
	sw_xmltree_t *xml;
	sw_xmlnode_t *node;
	sw_syscfg_t  *syscfg;
	sw_prdt_cfg_t *prdt_cfg;
	sw_prdt_cfg_t *ptr;

	if (path == NULL || cfghder == NULL)
	{
		pub_log_error("[%s][%d] read_lsn_cfg input argv error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	xml = cfg_read_xml(path->prdt_file);
	if (xml == NULL )
	{
		pub_log_error("[%s][%d] pub_xml_crtree [%s] error!",__FILE__,__LINE__,path->prdt_file);
		return SW_ERR;
	}

	node = pub_xml_locnode(xml,".DFISBP.PRODUCT");
	if (node == NULL )
	{
		pub_log_error("[%s][%d]  [%s] file must have .DFISBP.PRODUCT !",__FILE__,__LINE__,path->prdt_file);
		goto ERR_EXIT;
	}

	syscfg = (sw_syscfg_t *) BLOCKAT(cfghder,cfghder->sys_offset);
	prdt_cfg = (sw_prdt_cfg_t *) BLOCKAT(cfghder,cfghder->prdt_offset);
	ptr = prdt_cfg;
	do
	{	
		ret = read_prdt_cell(node,ptr);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] read_prdt_cell error!",__FILE__,__LINE__,path->prdt_file);
			goto ERR_EXIT;			
		}
		ptr++;
		node = node->next;
	}while(node && syscfg->prdt_max > (ptr-prdt_cfg));

	cfghder->prdt_use= ptr - prdt_cfg;
	if (cfghder->prdt_use >= syscfg->prdt_max)
	{
		pub_log_error("[%s][%d]  product configure in [%s]  is over the limit:[%d]!",
				__FILE__,__LINE__,path->prdt_file,syscfg->prdt_max);
		goto ERR_EXIT;
	}

	pub_xml_deltree(xml);
	return SW_OK;

ERR_EXIT:

	pub_xml_deltree(xml);
	return SW_ERR;
}

int cfg_add_prdt(sw_cfgshm_hder_t *cfghder, sw_char_t *prdt)
{
	sw_int_t	ret = 0;
	sw_char_t	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;
	sw_xmlnode_t	*node1 = NULL;
	sw_prdt_cfg_t	*prdt_cfg = NULL;
	sw_prdt_cfg_t	*ptr = NULL;

	if (cfghder == NULL)
	{
		pub_log_error("[%s][%d] cfg_add_prdt params error! shm is null!", __FILE__, __LINE__);
		return SW_ERR;
	}

	memset(xmlname, 0x0, sizeof(xmlname));
	sprintf(xmlname, "%s/%s/%s", getenv("SWWORK"), SW_CFG_PATH, SW_PRDT_FILE);
	pub_log_info("[%s][%d] xmlname=[%s]", __FILE__, __LINE__, xmlname);
	xml = cfg_read_xml(xmlname);
	if (xml == NULL )
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]", __FILE__, __LINE__, xmlname);
		return SW_ERR;
	}

	prdt_cfg = (sw_prdt_cfg_t *)BLOCKAT(cfghder, cfghder->prdt_offset);
	ptr = &prdt_cfg[cfghder->prdt_use];

	node = pub_xml_locnode(xml,".DFISBP.PRODUCT");
	if (node == NULL )
	{
		pub_log_error("[%s][%d] Not config PRODUCT!", __FILE__, __LINE__);
		goto ERR_EXIT;
	}
	while (node != NULL)
	{	
		if (strcmp(node->name, "PRODUCT") != 0)
		{
			node = node->next;
			continue;
		}

		xml->current = node;
		node1 = pub_xml_locnode(xml, "NAME");
		if (node1 == NULL || node1->value == NULL)
		{
			pub_log_error("[%s][%d] Not config NAME!", __FILE__, __LINE__);
			goto ERR_EXIT;
		}

		if (strcmp(node1->value, prdt) == 0)
		{
			break;
		}

		node = node->next;
	}
	if (node == NULL)
	{
		pub_log_error("[%s][%d] Not find prdt [%s]!", __FILE__, __LINE__, prdt);
		goto ERR_EXIT;
	}

	ret = read_prdt_cell(node, ptr);
	if (ret != SW_OK)
	{
		pub_log_error("[%s][%d] read_prdt_cell error!", __FILE__, __LINE__);
		goto ERR_EXIT;
	}
	cfghder->prdt_use++;
	pub_log_info("[%s][%d] prdtname=[%s] status=[%d]", __FILE__, __LINE__, ptr->prdtname, ptr->status);

	pub_xml_deltree(xml);
	return SW_OK;

ERR_EXIT:

	pub_xml_deltree(xml);
	return SW_ERR;
}

static sw_int_t read_prdt_cell(sw_xmlnode_t *node,sw_prdt_cfg_t *prdtcfg)
{
	sw_int32_t i;

	sw_xmlnode_t *curr_node;
	sw_xmlnode_t *tmp; 

	assert(node!= NULL );
	assert(prdtcfg != NULL );

	curr_node = node->firstchild;

	while(curr_node != NULL)
	{
		if(pub_mem_memcmp(curr_node->name,"STATUS",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]STATUS is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				prdtcfg->status = pub_str_atoi(curr_node->value,pub_str_strlen(curr_node->value));
			}
		}		
		else  if(pub_mem_memcmp(curr_node->name,"NAME",4)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]NAME is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				memcpy(prdtcfg->prdtname,curr_node->value,sizeof(prdtcfg->prdtname)-1);
			}
		}
		else if(pub_mem_memcmp(curr_node->name,"CHANNEL",7)==0)
		{
			i = 0;
			tmp = curr_node->firstchild;
			while (tmp && i < SW_LSN_PRDT_MAX)
			{
				if (strncmp(tmp->name,"LISTEN",6) != 0)
				{
					pub_log_error("[%s][%d] the [%s] is illegal !",__FILE__,__LINE__,tmp->name);
					tmp = tmp->next;
					continue;
				}
				if(tmp->value == NULL||strlen(tmp->value) == 0)
				{
					pub_log_error("[%s][%d]LISTEN is empty", __FILE__, __LINE__);
					tmp = tmp->next;
					continue;
				}
				else
				{
					memcpy(prdtcfg->lsnname[i],tmp->value,sizeof(prdtcfg->lsnname[i])-1);
				}

				i++;
				tmp = tmp->next;
			}

			prdtcfg->lsn_cnt = i;
		}		
		curr_node = curr_node->next;
	}

	return SW_OK;
}


static sw_int_t read_chl_cell(sw_xmlnode_t *node,sw_chl_cfg_t *chlcfg)
{
	sw_int32_t i;

	sw_xmlnode_t *curr_node;
	sw_xmlnode_t *tmp; 

	assert(node!= NULL );
	assert(chlcfg != NULL );

	curr_node = node->firstchild;

	while(curr_node != NULL)
	{
		if(pub_mem_memcmp(curr_node->name,"STATUS",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]STATUS is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				chlcfg->status = pub_str_atoi(curr_node->value,pub_str_strlen(curr_node->value));
			}
		}		
		else  if(pub_mem_memcmp(curr_node->name,"LISTEN",6)==0)
		{
			if(curr_node->value == NULL||strlen(curr_node->value) == 0)
			{
				pub_log_error("[%s][%d]LISTEN is empty", __FILE__, __LINE__);
				return SW_ERR;
			}
			else
			{
				strncpy(chlcfg->lsnname,curr_node->value,sizeof(chlcfg->lsnname)-1);
			}
		}
		else if(pub_mem_memcmp(curr_node->name,"PRODUCT",7) ==0)
		{
			i = 0;
			tmp = curr_node->firstchild;
			while (tmp && i < SW_LSN_PRDT_MAX)
			{
				if (strncmp(tmp->name,"NAME",4) != 0)
				{
					pub_log_error("[%s][%d] the [%s] is illegal !",__FILE__,__LINE__,tmp->name);
					tmp = tmp->next;
					continue;
				}
				if(tmp->value == NULL||strlen(tmp->value) == 0)
				{
					pub_log_error("[%s][%d]NAME is empty", __FILE__, __LINE__);
					tmp = tmp->next;
					continue;
				}
				else
				{
					strncpy(chlcfg->prdtname[i],tmp->value,sizeof(chlcfg->prdtname)-1);
				}

				i++;
				tmp = tmp->next;
			}

			chlcfg->prdt_cnt = i;
		}		
		curr_node = curr_node->next;
	}

	return SW_OK;
}
static sw_int_t read_chl_cfg(sw_global_path_t *path,sw_cfgshm_hder_t *cfghder)
{
	sw_int_t ret;
	sw_xmltree_t *xml;
	sw_xmlnode_t *node;
	sw_syscfg_t  *syscfg;
	sw_chl_cfg_t *chl_cfg;
	sw_chl_cfg_t *ptr;

	if (path == NULL || cfghder == NULL)
	{
		pub_log_error("[%s][%d] read_lsn_cfg input argv error!",__FILE__,__LINE__);
		return SW_ERR;
	}

	xml = cfg_read_xml(path->chl_file);
	if (xml == NULL )
	{
		pub_log_error("[%s][%d] pub_xml_crtree [%s] error!",__FILE__,__LINE__,path->chl_file);
		return SW_ERR;
	}

	node = pub_xml_locnode(xml,".DFISBP.CHANNEL");
	if (node == NULL )
	{
		pub_log_error("[%s][%d]  [%s] file must have PRODUCT !",__FILE__,__LINE__,path->chl_file);
		goto ERR_EXIT;
	}

	syscfg = (sw_syscfg_t *) BLOCKAT(cfghder,cfghder->sys_offset);
	chl_cfg = (sw_chl_cfg_t *) BLOCKAT(cfghder,cfghder->chl_offset);
	ptr = chl_cfg;
	do
	{	
		ret = read_chl_cell(node,ptr);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] read_chl_cell error!",__FILE__,__LINE__,path->chl_file);
			goto ERR_EXIT;			
		}
		ptr++;
		node = node->next;
	}while(node && syscfg->lsn_max > (ptr-chl_cfg));
	cfghder->lsn_use = ptr - chl_cfg;

	if (cfghder->lsn_use >= syscfg->lsn_max)
	{
		pub_log_error("[%s][%d]  channel configure in [%s]  is over the limit:[%d]!",__FILE__,__LINE__,path->chl_file,syscfg->lsn_max);
		goto ERR_EXIT;
	}

	pub_xml_deltree(xml);
	return SW_OK;

ERR_EXIT:
	pub_xml_deltree(xml);
	return SW_ERR;
}

sw_int_t cfg_load_key(const sw_global_path_t *path, key_t *key)
{
	sw_int_t	result = SW_ERROR;

	if (path == NULL || key == NULL)
	{
		pub_log_error("%s, %d, Param error.", __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = pub_key_load((const char *)path->key_file, key, 1);
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pub_key_load error, key_file[%s].",__FILE__,__LINE__,path->key_file);
		return SW_ERROR;
	}

	return SW_OK;
}

/******************************************************************************
 ** Name : cfg_get_prdt_cfg
 ** Desc : Get product configuration address in share-memory
 ** Input: 
 **     addr: The address of shm_cfg. [addr = cycle->shm_cfg]
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
const sw_prdt_cfg_t *cfg_get_prdt_cfg(const sw_cfgshm_hder_t *shmcfg)
{
	return (sw_prdt_cfg_t *)BLOCKAT(shmcfg, shmcfg->prdt_offset);
}

const sw_chl_cfg_t *cfg_get_chnl_cfg(const sw_cfgshm_hder_t *shmcfg)
{
	return (sw_chl_cfg_t *)BLOCKAT(shmcfg, shmcfg->chl_offset);
}

/******************************************************************************
 ** Name : cfg_search_prdt_cfg
 ** Desc : Search product configuration
 ** Input: 
 **     addr: The address of shm_cfg. [addr = cycle->shm_cfg]
 ** Output: NONE
 ** Return: 0:success !0:failed
 ** Process:
 ** Note :
 ** Author: # Qifeng.zou # 2013.06.20 #
 ******************************************************************************/
const sw_prdt_cfg_t *cfg_search_prdt_cfg(const sw_cfgshm_hder_t *shmcfg, const char *prdt_name)
{
	sw_int_t	idx = 0;
	sw_prdt_cfg_t	*item = NULL;
	
	if (shmcfg == NULL || prdt_name == NULL || prdt_name[0] == '\0')
	{
		pub_log_error("[%s][%d] Input param error!", __FILE__, __LINE__);
		return NULL;
	}
	
	item = (sw_prdt_cfg_t *)BLOCKAT(shmcfg, shmcfg->prdt_offset);
	for(idx=0; idx<shmcfg->prdt_use; idx++)
	{
		if(0 == strcasecmp(item->prdtname, prdt_name))
		{
			return item;
		}
		item++;
	}

	return NULL;
}

sw_dbcfg_t *cfg_get_db_cfg(sw_cfgshm_hder_t *shmaddr)
{	
	pub_log_debug("[%s][%d] db_offet====[%d]", __FILE__, __LINE__, shmaddr->db_offset);
	return (sw_dbcfg_t *)((char *)shmaddr + shmaddr->db_offset);
}

int cfg_get_dbcnt(sw_cfgshm_hder_t *shmaddr)
{	
	return shmaddr->db_use;
}

int cfg_get_db_conn(sw_cfgshm_hder_t *shmaddr, sw_char_t *name, sw_dbcfg_t *db)
{
	int	i = 0;
	int	db_cnt = 0;
	sw_dbcfg_t	*dbcfg = NULL;

	db_cnt = cfg_get_dbcnt(shmaddr);
	dbcfg = cfg_get_db_cfg(shmaddr);
	pub_log_info("[%s][%d] dbcfg=[%x] db_cnt=[%d]", __FILE__, __LINE__, dbcfg, db_cnt);
	for (i = 0; i < db_cnt; i++)
	{
		pub_log_info("[%s][%d] index=[%d] dbid=[%s] dbname=[%s]", __FILE__, __LINE__, i, dbcfg[i].dbid, dbcfg[i].dbname);
		if (strcmp(dbcfg[i].dbid, name) == 0)
		{
			memcpy(db, &dbcfg[i], sizeof(sw_dbcfg_t));
			return SW_OK;
		}
	}
	pub_log_error("[%s][%d] Can not find [%s] db info!", __FILE__, __LINE__, name);
	return SW_ERROR;
}

int print_db_info(sw_cfgshm_hder_t *shmaddr)
{
	int	i = 0;
	int	db_cnt = 0;
	sw_dbcfg_t	*dbcfg = NULL;

	db_cnt = cfg_get_dbcnt(shmaddr);
	dbcfg = cfg_get_db_cfg(shmaddr);
	pub_log_info("[%s][%d] dbcfg=[%x] db_cnt=[%d]", __FILE__, __LINE__, dbcfg, db_cnt);
	for (i = 0; i < db_cnt; i++)
	{
		pub_log_info("[%s][%d] index=[%d] dbid=[%s] dbname=[%s]", 
				__FILE__, __LINE__, i, dbcfg[i].dbid, dbcfg[i].dbname);
	}
	return 0;
}

ssize_t pub_cfg_atosz(u_char *line, size_t n)
{
	ssize_t  value;

	if (n == 0)
	{
		return -1;
	}

	for (value = 0; n--; line++)
	{
		if (*line < '0' || *line > '9')
		{
			return -1;
		}

		value = value * 10 + (*line - '0');
	}

	return value;
}

ssize_t pub_cfg_parse_size(char *line)
{
	char    unit;
	size_t  len;
	ssize_t size, scale;

	len = strlen(line);
	unit = line[len - 1];

	switch (unit)
	{
		case 'K':
		case 'k':
			len--;
			scale = 1024;
			break;

		case 'M':
		case 'm':
			len--;
			scale = 1024 * 1024;
			break;

		default:
			scale = 1;
	}

	size = pub_cfg_atosz((u_char *)line, len);
	if (size == -1)
	{
		return -1;
	}

	size *= scale;

	return size;
}

static sw_int_t read_alog_cfg(sw_xmlnode_t *pnode, alog_cfg_t *alog)
{
	sw_xmlnode_t	*node = pnode->firstchild;

	alog->use = 0;
	alog->transport = SW_TCP_TRANSPORT;
	alog->wait_time = 300;
	alog->buffer_size = 0;
	alog->shm_alloc_size = DEFAULT_SHM_ALLOC_SIZE;
	memset(alog->trc_prefix, 0x0, sizeof(alog->trc_prefix));

	alog->proc_num = 1;
	alog->pthread_num = 2;
	alog->timeout = 120;
	while (node != NULL)
	{
		if (strcmp(node->name, "USE") == 0)
		{
			if (node->value != NULL)
			{
				alog->use = atoi(node->value);
			}
			if (alog->use == 0)
			{
				pub_log_info("[%s][%d] not use logsvr", __FILE__, __LINE__);
				return 0;
			}
		}

		if (strcmp(node->name, "TRANSPORT") == 0)
		{
			if (node->value != NULL)
			{
				if (strcasecmp(node->value, "UNIX") == 0)
				{
					alog->transport = SW_UNIX_TRANSPORT;
				}
			}
		}

		if (strcmp(node->name, "IP") == 0)
		{
			if (node->value != NULL)
			{
				strncpy(alog->ip, node->value, sizeof(alog->ip) - 1);
			}
		}

		if (strcmp(node->name, "PORT") == 0)
		{
			if (node->value != NULL)
			{
				alog->port = atoi(node->value);
			}
		}

		if (strcmp(node->name, "UPATH") == 0)
		{
			if (node->value != NULL)
			{
				strncpy(alog->upath, node->value, sizeof(alog->upath) - 1);
			}
		}

		if (strcmp(node->name, "WAITTIME") == 0)
		{
			if (node->value != NULL)
			{
				alog->wait_time = atoi(node->value);


			}
		}
		if (strcmp(node->name, "PROCNUM") == 0)
		{
			if (node->value != NULL)
			{
				alog->proc_num = atoi(node->value);
			}
		}

		if (strcmp(node->name, "PTHREADNUM") == 0)
		{
			if (node->value != NULL && atoi(node->value) >= 2 )
			{
				alog->pthread_num = atoi(node->value);
			}
		}

		if (strcmp(node->name, "TIMEOUT") == 0)
		{
			if (node->value != NULL)
			{
				alog->timeout = atoi(node->value);
			}
		}

		if (strcmp(node->name, "LOGCACHESIZE") == 0)
		{
			if (node->value != NULL && node->value[0] != '\0')
			{
				alog->buffer_size = pub_cfg_parse_size(node->value);
			}
		}

		if (strcmp(node->name, "SHMALLOCSIZE") == 0)
		{
			if (node->value != NULL && node->value[0] != '\0')
			{
				alog->shm_alloc_size = pub_cfg_parse_size(node->value);
			}
		}
		if (strcmp(node->name, "TRCPREFIX") ==  0)
		{
			if (node->value != NULL)
			{
				strncpy(alog->trc_prefix, node->value, sizeof(alog->trc_prefix) - 1);
			}
		}

		node = node->next;
	}
	pub_log_info("[%s][%d] use=[%d] ip=[%s] port=[%d] buffer_size=[%ld] prefix=[%s]",
			__FILE__, __LINE__, alog->use, alog->ip, alog->port, alog->buffer_size, alog->trc_prefix);

	return 0;
}

static sw_int_t read_ares_cfg(sw_xmlnode_t *pnode, ares_cfg_t *ares)
{
	sw_xmlnode_t	*node = pnode->firstchild;

	ares->use = 0;
	ares->mtype_use = 0;
	ares->seqs_use = 0;
	ares->mon_use = 1;
	ares->wait_time = 300;

	while (node != NULL)
	{
		if (strcmp(node->name, "USE") == 0)
		{
			if (node->value != NULL)
			{
				ares->use = atoi(node->value);
			}
		}

		if (strcmp(node->name, "IP") == 0)
		{
			if (node->value != NULL)
			{
				strncpy(ares->ip, node->value, sizeof(ares->ip) - 1);
			}
		}

		if (strcmp(node->name, "PORT") == 0)
		{
			if (node->value != NULL)
			{
				ares->port = atoi(node->value);
			}
		}

		if (strcmp(node->name, "MTYPEUSE") == 0)
		{
			if (node->value != NULL)
			{
				ares->mtype_use = atoi(node->value);
			}
		}

		if (strcmp(node->name, "SEQSUSE") == 0)
		{
			if (node->value != NULL)
			{
				ares->seqs_use = atoi(node->value);
			}
		}

		if (strcmp(node->name, "MONUSE") == 0)
		{
			if (node->value != NULL)
			{
				ares->mon_use = atoi(node->value);
			}
		}

		if (strcmp(node->name, "WAITTIME") == 0)
		{
			if (node->value != NULL)
			{
				ares->wait_time = atoi(node->value);
			}
		}
		node = node->next;
	}
	pub_log_info("[%s][%d] ARES, USE=[%d] IP=[%s] PORT=[%d] SEQUSE=[%d]",
			__FILE__, __LINE__, ares->use, ares->ip, ares->port, ares->seqs_use);	

	return 0;
}

sw_int_t cfg_get_alog_cfg(sw_cfgshm_hder_t*addr, alog_cfg_t *alog)
{
	pub_mem_memcpy(alog, BLOCKAT(addr, addr->alog_offset), sizeof(alog_cfg_t));
	pub_log_info("[%s][%d] alog:use=[%d] ip=[%s] port=[%d]",
			__FILE__, __LINE__, alog->use, alog->ip, alog->port);

	return SW_OK;
}

sw_int_t cfg_get_ares_cfg(sw_cfgshm_hder_t*addr, ares_cfg_t *ares)
{
	pub_mem_memcpy(ares, BLOCKAT(addr, addr->ares_offset), sizeof(ares_cfg_t));
	pub_log_info("[%s][%d] ares:use=[%d] ip=[%s] port=[%d]",
			__FILE__, __LINE__, ares->use, ares->ip, ares->port);

	return SW_OK;
}

int pub_cfg_get_eswid(char *eswid)
{
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));

	sprintf(xmlname, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_info("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".SWCFG.GLOBAL.ESWSID");
	if (node != NULL && node->value != NULL)
	{
		strcpy(eswid, node->value);
		pub_log_info("[%s][%d] ESWID=[%s]", __FILE__, __LINE__, node->value);
	}
	pub_xml_deltree(xml);

	return SW_OK;
}

int pub_cfg_get_workmode(char *workmode)
{
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));

	sprintf(xmlname, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".SWCFG.GLOBAL.WORKMODE");
	if (node != NULL && node->value != NULL)
	{
		strcpy(workmode, node->value);
		pub_log_info("[%s][%d] WORKMODE=[%s]", __FILE__, __LINE__, node->value);
	}
	pub_xml_deltree(xml);

	return SW_OK;
}

sw_xmltree_t *cfg_read_xml(sw_char_t *xmlname)
{
	int     ret = 0;
	sw_buf_t        xmlbuf;
	sw_xmltree_t    *xml = NULL;

	memset(&xmlbuf, 0x0, sizeof(xmlbuf));

	pub_buf_init(&xmlbuf);
	ret = readfile(xmlname, &xmlbuf);
	if (ret < 0)
	{
		pub_buf_clear(&xmlbuf);
		pub_log_error("[%s][%d] Read file [%s] error!", __FILE__, __LINE__, xmlname);
		return NULL;
	}
	xml = pub_xml_crtree_ext(xmlbuf.data, xmlbuf.len);
	if (xml == NULL)
	{
		pub_buf_clear(&xmlbuf);
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, xmlname);
		return NULL;
	}
	pub_buf_clear(&xmlbuf);

	return xml;
}

int pub_cfg_get_logmode(char *logmode)
{
	char	xmlname[128];
	sw_xmltree_t	*xml = NULL;
	sw_xmlnode_t	*node = NULL;

	memset(xmlname, 0x0, sizeof(xmlname));

	sprintf(xmlname, "%s/cfg/swconfig.xml", getenv("SWWORK"));
	xml = pub_xml_crtree(xmlname);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] Create xml tree error! xmlname=[%s]",
				__FILE__, __LINE__, xmlname);
		return SW_ERROR;
	}

	node = pub_xml_locnode(xml, ".SWCFG.ALOG.USE");
	if (node != NULL && node->value != NULL && node->value[0] == '1')
	{
		node = pub_xml_locnode(xml, ".SWCFG.ALOG.TRANSPORT");
		if (node != NULL && node->value != NULL && strcasecmp(node->value, "UNIX") == 0)
		{
			strcpy(logmode, "1");
		}
	}
	pub_xml_deltree(xml);

	return SW_OK;
}

