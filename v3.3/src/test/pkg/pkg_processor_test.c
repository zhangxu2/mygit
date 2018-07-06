#include "pkg_processor.h"

int main(int argc, char* argv[])
{
	sw_int_t	result = SW_ERROR;
	sw_pkg_processor_t	processor;

	pub_mem_memzero(&processor, sizeof(processor));
	pkg_processor_init(&processor);

	result = pkg_processor_load_lib(&processor, "SWTCPSS.so");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_processor_set_lib error."
			, __FILE__, __LINE__);
		return SW_ERROR;
	}

	result = pkg_link_analyze_func(&processor, "SWTCPSS");
	if (result != SW_OK)
	{
		pub_log_error("%s, %d, pkg_link_analyze_func error."
			, __FILE__, __LINE__);
		return SW_ERROR;		
	}
	
	pkg_processor_reset(&processor);
	
	return SW_OK;
}

