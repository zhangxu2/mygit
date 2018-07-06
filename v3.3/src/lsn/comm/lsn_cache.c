#include "lsn_prdt.h"
#include "lsn_pub.h"

sw_xmltree_t *lsn_get_xmltree(sw_lsn_cycle_t *cycle, char *prdt, char *name)
{
	sw_char_t	xmlname[128];
	
	memset(xmlname, 0x0, sizeof(xmlname));
	if (prdt == NULL || prdt[0] == '\0')
	{
		sprintf(xmlname, "%s/cfg/common/%s", cycle->base.work_dir.data, name);
	}
	else
	{
		sprintf(xmlname, "%s/products/%s/etc/lsncfg/common/%s", cycle->base.work_dir.data, prdt, name);
	}
	
	return cfg_read_xml(xmlname);
}

sw_int_t lsn_get_plugin_handle(sw_lsn_cycle_t *cycle, sw_char_t *name, sw_char_t *prdt_name, void **hd, void **df)
{
	void 		*handle;
	void		*dllfun;
	sw_char_t	*ptr = NULL;
	sw_char_t	libso[128];
	sw_char_t	funname[32];
	sw_char_t	path[128];
	sw_char_t	libname[128];
	
	memset(path, 0x0, sizeof(path));
	memset(libso, 0x0, sizeof(libso));
	memset(funname, 0x0, sizeof(funname));
	memset(libname, 0x0, sizeof(libname));
	
	if (strstr(name, "/") != NULL)
	{
		ptr = strchr(name, '/');
		if (ptr == NULL)
		{
			pub_log_error("[%s][%d] name error!", __FILE__, __LINE__);
			return SW_ERROR;
		}
		strncpy(libname, name, ptr - name);
		strcpy(funname, ptr + 1);
		
		sprintf(libso, "%s/plugin/%s", cycle->base.work_dir.data, libname);
		if (!pub_file_exist(libso))
		{
			memset(libso, 0x0, sizeof(libso));
			sprintf(libso, "%s/plugin/%s", cycle->base.home_dir.data, libname);
			if (!pub_file_exist(libso))
			{
				pub_log_error("[%s][%d] libso [%s] is not exist!", __FILE__, __LINE__, libso);	
				return SW_ERROR;
			}
		}
	}
	else
	{
		sprintf(libso, "%s/plugin/libfile.so", cycle->base.work_dir.data);
		if (!pub_file_exist(libso))
		{
			memset(libso, 0x0, sizeof(libso));
			sprintf(libso, "%s/plugin/libfile.so", cycle->base.home_dir.data);
			if (!pub_file_exist(libso))
			{
				pub_log_error("[%s][%d] libso [%s] is not exist!", __FILE__, __LINE__, libso);	
				return SW_ERROR;
			}
		}
		sprintf(funname, "%s", name);
	}
	
	handle = (void *)dlopen(libso, RTLD_LAZY | RTLD_GLOBAL);
	if (handle == NULL)
	{
		pub_log_error("[%s][%d] dlopen [%s] error! error:[%s]", __FILE__, __LINE__, libso, dlerror());
		return SW_ERROR;
	}
	
	dllfun = (sw_int_t (*)())dlsym(handle, funname);
	if (dllfun == NULL)
	{
		dlclose(handle);
		pub_log_error("[%s][%d] dlsym [%s][%s] error! error:[%s]", 
			__FILE__, __LINE__, libso, funname, dlerror());
		return SW_ERROR;
	}
	*hd = handle;
	*df = dllfun;
	
	return SW_OK;
}

sw_int_t lsn_get_all_cache(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	int	ret = 0;
	sw_chnl_t	*chnl = NULL;
	sw_route_t	*route = NULL;

	if (cycle == NULL)
	{
		pub_log_error("[%s][%d] Param error!", __FILE__, __LINE__);
		return SW_ERROR;
	}
	
	chnl = &cycle->chnl;
	chnl->fun.pkgmap_func = NULL;	
	chnl->fun.file_func = NULL;	
	chnl->fun.svrmap_func = NULL;	
	chnl->fun.deny_func = NULL;	

	/*** Get all tree ***/
	chnl->cache.pkgdeal = lsn_get_xmltree(cycle, NULL, cycle->lsn_conf.pkgcfg);
	if (chnl->cache.pkgdeal == NULL)
	{
		pub_log_error("[%s][%d] Get pkgdeal tree [%s] error!",
			__FILE__, __LINE__, cycle->lsn_conf.pkgcfg);
		return SW_ERROR;
	}
	
	if (cycle->lsn_conf.mapcfg[0] != '\0')
	{
		chnl->cache.pkgmap = lsn_get_xmltree(cycle, NULL, cycle->lsn_conf.mapcfg);
		if (chnl->cache.pkgmap == NULL)
		{
			pub_log_error("[%s][%d] Get pkgmap tree [%s] error!", 
				__FILE__, __LINE__, cycle->lsn_conf.mapcfg);
			return SW_ERROR;
		}
	}
		
	if (cycle->lsn_conf.factoranayly[0] != '\0')
	{
		chnl->cache.preana = lsn_get_xmltree(cycle, NULL, cycle->lsn_conf.factoranayly);
		if (chnl->cache.preana == NULL)
		{
			pub_log_error("[%s][%d] Get prean tree [%s] error!",
				__FILE__, __LINE__, cycle->lsn_conf.factoranayly);
			return SW_ERROR;
		}
	}

	if (cycle->lsn_conf.denycfg[0] != '\0')
	{
		chnl->cache.deny = lsn_get_xmltree(cycle, NULL, cycle->lsn_conf.denycfg);
		if (chnl->cache.deny == NULL)
		{	
			pub_log_error("[%s][%d] Get deny tree [%s] error!", 
				__FILE__, __LINE__, cycle->lsn_conf.denycfg);
			return SW_ERROR;
		}
	}
		
	/*** Get all handle ***/
	if (cycle->lsn_conf.filedeal[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, cycle->lsn_conf.filedeal, NULL, 
			&chnl->handle.file_handle, (void *)&chnl->fun.file_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] Get [%s] handle error!", 
				__FILE__, __LINE__, cycle->lsn_conf.filedeal);
			return SW_ERROR;
		}
	}
		
	if (cycle->lsn_conf.mapcfg[0] != '\0')
	{
		chnl->fun.pkgmap_func = map;
	}
		
	if (cycle->lsn_conf.svrmap[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, cycle->lsn_conf.svrmap, NULL, 
			&chnl->handle.svrmap_handle, (void *)&chnl->fun.svrmap_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!", 
				__FILE__, __LINE__, cycle->lsn_conf.svrmap);
			return SW_ERROR;
		}
	}
		
	if (cycle->lsn_conf.denyservice[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, cycle->lsn_conf.denyservice, NULL, 
			&chnl->handle.deny_handle, (void *)&chnl->fun.deny_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!", 
				__FILE__, __LINE__, cycle->lsn_conf.denyservice);
			return SW_ERROR;
		}
	}
	
	if (cycle->lsn_conf.startfunc[0] != '\0')
	{
		ret = lsn_get_plugin_handle(cycle, cycle->lsn_conf.startfunc, NULL, 
			&chnl->handle.start_handle, (void *)&chnl->fun.start_func);
		if (ret != SW_OK)
		{
			pub_log_error("[%s][%d] get [%s] handle error!", 
				__FILE__, __LINE__, cycle->lsn_conf.denyservice);
			return SW_ERROR;
		}
	}
	
	pub_log_debug("[%s][%d] route_cnt=[%d]", __FILE__, __LINE__, cycle->routes->head.route_cnt);
	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		route = &cycle->routes->route[i];
		route->fun.code_func = NULL;
		route->fun.type_func = NULL;
		route->fun.prean_func = NULL;
		route->fun.recov_func = NULL;
		route->fun.after_func = NULL;
		route->fun.synres_func = NULL;

		if (cycle->lsn_conf.svrmapcfg[0] != '\0')
		{
			route->cache.svrmap = lsn_get_xmltree(cycle, route->name, cycle->lsn_conf.svrmapcfg);
			if (route->cache.svrmap == NULL)
			{
				pub_log_error("[%s][%d] Get svrmap tree [%s] error!", 
					__FILE__, __LINE__, cycle->lsn_conf.svrmapcfg);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.codeconvert[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.codeconvert, route->name, 
				&route->handle.code_handle, (void *)&route->fun.code_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.codeconvert);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.typeanalyze[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.typeanalyze, route->name, 
				&route->handle.type_handle, (void *)&route->fun.type_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.typeanalyze);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.preanalyze[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.preanalyze, route->name, 
				&route->handle.prean_handle, (void *)&route->fun.prean_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.preanalyze);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.synres[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.synres, route->name, 
				&route->handle.synres_handle, (void *)&route->fun.synres_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.synres);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.recover[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.recover, route->name, 
				&route->handle.recov_handle, (void *)&route->fun.recov_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.recover);
				return SW_ERROR;
			}
		}

		if (route->prdt_cfg.afterdeal[0] != '\0')
		{
			ret = lsn_get_plugin_handle(cycle, route->prdt_cfg.afterdeal, route->name, 
				&route->handle.after_handle, (void *)&route->fun.after_func);
			if (ret != SW_OK)
			{
				pub_log_error("[%s][%d] get [%s] handle error!", 
					__FILE__, __LINE__, route->prdt_cfg.recover);
				return SW_ERROR;
			}
		}
	}
	
	pub_log_info("[%s][%d] get_all_cache success!", __FILE__, __LINE__);
	return SW_OK;
}

sw_int_t lsn_free_all_cache(sw_lsn_cycle_t *cycle)
{
	int	i = 0;
	sw_chnl_t	*chnl = NULL;
	sw_route_t	*route = NULL;

	chnl = &cycle->chnl;

	pub_xml_deltree(chnl->cache.pkgdeal);
	
	if (cycle->lsn_conf.mapcfg[0] != '\0')
	{
		pub_xml_deltree(chnl->cache.pkgmap);
	}
		
	if (cycle->lsn_conf.factoranayly[0] != '\0')
	{
		pub_xml_deltree(chnl->cache.preana);
	}

	if (cycle->lsn_conf.denycfg[0] != '\0')
	{
		pub_xml_deltree(chnl->cache.deny);
	}
		
	/*** Get all handle ***/
	if (cycle->lsn_conf.filedeal[0] != '\0')
	{
		dlclose(chnl->handle.file_handle);
	}
		
	if (cycle->lsn_conf.svrmap[0] != '\0')
	{
		dlclose(chnl->handle.svrmap_handle);
	}
		
	if (cycle->lsn_conf.denyservice[0] != '\0')
	{
		dlclose(chnl->handle.deny_handle);
	}
	
	if (cycle->lsn_conf.startfunc[0] != '\0')
	{
		dlclose(chnl->handle.start_handle);
	}
	
	pub_log_debug("[%s][%d] route_cnt=[%d]", __FILE__, __LINE__, cycle->routes->head.route_cnt);
	for (i = 0; i < cycle->routes->head.route_cnt; i++)
	{
		route = &cycle->routes->route[i];
	
		if (cycle->lsn_conf.svrmapcfg[0] != '\0' && route->cache.svrmap != NULL)
		{
			pub_xml_deltree(route->cache.svrmap);
		}

		if (route->prdt_cfg.codeconvert[0] != '\0')
		{
			dlclose(route->handle.code_handle);
		}

		if (route->prdt_cfg.typeanalyze[0] != '\0')
		{
			dlclose(route->handle.type_handle);
		}

		if (route->prdt_cfg.preanalyze[0] != '\0')
		{
			dlclose(route->handle.prean_handle);
		}

		if (route->prdt_cfg.synres[0] != '\0')
		{
			dlclose(route->handle.synres_handle);
		}

		if (route->prdt_cfg.recover[0] != '\0')
		{
			dlclose(route->handle.recov_handle);
		}

		if (route->prdt_cfg.afterdeal[0] != '\0')
		{
			dlclose(route->handle.after_handle);
		}
	}
	if (cycle->handle)
	{
		dlclose(cycle->handle);
		pub_log_info("[%s][%d] dlclose handle!", __FILE__, __LINE__);
	}
	
	pkg_release_list(&(cycle->cache_buf));

	pub_log_info("[%s][%d] free all cache success!", __FILE__, __LINE__);

	return SW_OK;
}

int lsn_cache_upd_svrmap(sw_lsn_cycle_t *cycle, int index)
{
	sw_route_t	*route = NULL;
	
	if (cycle == NULL || index < 0)
	{
		pub_log_error("[%s][%d Param error!", __FILE__, __LINE__);
		return -1;
	}
	
	route = &cycle->routes->route[index];
	if (cycle->lsn_conf.svrmapcfg[0] != '\0')
	{
		route->cache.svrmap = lsn_get_xmltree(cycle, route->name, cycle->lsn_conf.svrmapcfg);
		if (route->cache.svrmap == NULL)
		{
			pub_log_error("[%s][%d] Get svrmap tree [%s] error!",
				__FILE__, __LINE__, cycle->lsn_conf.svrmapcfg);
			return SW_ERROR;
		}
	}

	return SW_OK;
}

