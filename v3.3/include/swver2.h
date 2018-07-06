#ifndef __SWVER2_H__
#define __SWVER2_H__
#include "swxml.h"

#define zip_space(s)              pub_str_zipspace(s)
#define nSetXmlVar(varname,value) set_xml_var(varname, value)
#define nGetField_len(name)       get_field_len(name)
#define nGetXmlVar(varname,value) get_xml_var(varname,value)
#define nCpFieldZip(from,to,desc) cp_field_zip(from,to,desc)
#define nCpField(from,to,desc)    cp_field(from,to,desc)
#define vClearExtVars()		  extvars_clear()
#define nGetVariable(name,value)  get_data_len(name,value)
#define nSetVariable(name,type,value,length) set_variable(name,type,value,length)
#define nSetVarString(name,value) set_zd_data(name,value)
#define nSetVarBinary(name,value,length) set_zd_data_len(name,value,length)
#define checkdirp(path)	pub_file_check_dir(path)

/*** XML ***/
#define TEXTVAR	sw_xmlnode_t
#define S_XMLTREE	sw_xmltree_t
#define S_XMLNODE	sw_xmlnode_t
#define s_crxmltree(filename)     pub_xml_crtree(filename)
#define s_locxmlnode(xml, loc)    pub_xml_locnode(xml, loc)
#define s_delxmltree(xml)         pub_xml_deltree(xml)

#endif
