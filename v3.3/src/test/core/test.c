#include "pub_type.h"
#include "pub_xml.h"

int main()
{

	sw_xmltree_t *xml;
	
	xml = pub_xml_crtree("t.xml");
	
	if (xml == NULL)
	{
		printf("crtree error!\n");
		return 0;
	}

	pub_xml_prt(xml->root);

	pub_xml_deltree(xml);

	return 0;

}
	
