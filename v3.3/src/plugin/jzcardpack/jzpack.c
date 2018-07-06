#include "pub_log.h"
#include "pub_vars.h"
#include "pub_xml.h"

extern sw_xmltree_t *pkg_read_xml(sw_char_t *xmlname);

/**
������:pack_a
����: ������п���ʽ
����:
	������ļ�ָ��(��xml�ĵ�ǰ���)
	xml���(���ڿ������TAB�ĸ���)
����ֵ:
	0	�ɹ�
	-1	�������
˵��:
	����ת����Ӧ��ϵ:
		1.field��ֱ�Ӱ�����ת��
		2.struct��ֱ�ӽ������е�����
		3.array���Ȳ鿴����Ԫ���Ƿ��ڱ������д���,�������ֱ�Ӱ�����ѭ�����
			��������ڲ鿴�ļ�dat/$date_$traceno_����.dat�ļ��Ƿ����,������
			�����¸�ʽת��:
			��һ�У�����1,����2,....
			��n ��: ����1,����2,...
**/

int pack_a(sw_loc_vars_t *vars, FILE *fp, int level, sw_xmltree_t *xml)
{
	int	i = 0;
	int	scale = 0;
	int	len = 0;
	int	length = 0;
	char	tmp[128];
	char	name[128];
	char	value[2048];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node3 = NULL;
	sw_xmlnode_t	*node4 = NULL;
	sw_xmlnode_t	*node_bak = NULL;
	
	memset(tmp, 0x0, sizeof(tmp));
	memset(name, 0x0, sizeof(name));
	memset(value, 0x0, sizeof(value));
	
	node_bak = xml->current;
	if (strcmp(node_bak->name, "data") != 0)
	{
		pub_log_error("[%s][%d] �������[%s]����data!", __FILE__, __LINE__, node_bak->name);
		xml->current = node_bak;
		return -1;
	}
	
	if ((node1 = pub_xml_locnode(xml, "field")) != NULL)
	{
		/*** field���� ***/
		node2 = pub_xml_locnode(xml, "name");
		if (node2 == NULL)
		{
			pub_log_error("[%s][%d] data��δָ��name!", __FILE__, __LINE__);
			xml->current = node_bak;
			return -1;
		}
		
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		name[0] = '#';
		strcpy(name + 1, node2->value);
		
		node2 = pub_xml_locnode(xml, "vname");
		if (node2 != NULL && node2->value != NULL)
		{
			loc_get_zd_data(vars, node2->value, value);
			pub_log_info("[%s][%d] ȡƽ̨����[%s]=[%s]", __FILE__, __LINE__, node2->value, value);
		}
		else
		{
			loc_get_zd_data(vars, name, value);
			pub_log_info("[%s][%d] ȡƽ̨����[%s]=[%s]", __FILE__, __LINE__, name, value);
		}
		pub_str_zipspace(value);

		xml->current = node1;
		node2 = pub_xml_locnode(xml, "type");
		if (node2 == NULL)
		{
			pub_log_error("[%s][%d] [%s]ָ����������Ϊfield��δָ��type!", __FILE__, __LINE__, name);
			xml->current = node_bak;
			return -1;
		}

		if (strcmp(node2->value, "string") == 0)
		{
			/*** field-string���� ***/
			scale = 0;
			len = strlen(value);

			node3 = pub_xml_locnode(xml, "length");
			if(node3 == NULL)
			{
				length = len;
			}
			else
			{
				length = atoi(node3->value);
			}
			/*** ��ʵ�ʳ����� ***/
			length = len;

			/*** ��� ***/
			for (i = 0; i < level; i++)
			{
				fprintf(fp, "\t");
			}
			fprintf(fp, "<data name=\"%s\">\n", name + 1);
			
			for (i = 0; i < level + 1; i++)
			{
				fprintf(fp, "\t");
			}

			if (value[0] == '\0')
			{
				/*** ��������ر���δ��ֵ,���Բ���ģ���Ƿ�������Ĭ��ֵ ***/
				if (node1->value != NULL && node1->value[0] != '\0')
				{
					length = strlen(node1->value);
					fprintf(fp, "<field type=\"string\" length=\"%d\" scale=\"0\" >%s</field>\n", length, node1->value);
				}
				else
				{
					fprintf(fp,"<field type=\"string\" length=\"%d\" scale=\"0\" />\n", length);
				}
			}
			else
			{
				fprintf(fp, "<field type=\"string\" length=\"%d\" scale=\"0\" >%s</field>\n", length, value);
			}

			for (i = 0; i < level; i++)
			{
				fprintf(fp, "\t");
			}
			fprintf(fp, "</data>\n");
		}
		else if (strcmp(node2->value, "double") == 0)
		{
			/*** field-string���� ***/
			scale = 0;
			len = strlen(value);

			node3 = pub_xml_locnode(xml, "length");
			if (node3 == NULL)
			{
				length = 0;
			}
			else
			{
				length = atoi(node3->value);
			}

			node3 = pub_xml_locnode(xml, "scale");
			if (node3 == NULL)
			{
				scale = 0;
			}
			else
			{
				scale = atoi(node3->value);
			}

			/*** ��� ***/
			for (i = 0; i < level; i++)
			{
				fprintf(fp, "\t");
			}			
			fprintf(fp, "<data name=\"%s\">\n", name + 1);
			for (i = 0; i < level + 1; i++)
			{
				fprintf(fp, "\t");
			}
			/*** TODO:�����ͳ���Ҳ�ĳ��Զ����� ***/
			length = 0;
			if (length > 0)
			{
				sprintf(tmp, "%*.*f", length, scale, atof(value));
				length = strlen(tmp);
			}
			else
			{
				sprintf(tmp, "%.*f", scale, atof(value));
				length = strlen(tmp);
			}
			fprintf(fp, "<field type=\"double\" length=\"%d\" scale=\"%d\" >%s</field>\n", length, scale, value);
			for (i = 0; i < level; i++)
			{
				fprintf(fp, "\t");
			}
			fprintf(fp, "</data>\n");
		}
		else
		{
			pub_log_error("[%s][%d] [%s]ָ����������[%s]��֧��!", __FILE__, __LINE__, name + 1, node2->value);
			xml->current = node_bak;
			return -1;
		}
	}
	else if ((node1 = pub_xml_locnode(xml, "struct")) != NULL)
	{
		/*** struct���� ***/
		node2 = pub_xml_locnode(xml, "name");
		if (node2 == NULL)
		{
			pub_log_error("[%s][%d] data��δָ��name!", __FILE__, __LINE__);
			xml->current = node_bak;
			return -1;
		}
	
		memset(name, 0x0, sizeof(name));
		memset(value, 0x0, sizeof(value));
		name[0] = '#';
		strcpy(name + 1, node2->value);

		for (i = 0; i < level; i++)
		{
			fprintf(fp, "\t");
		}

		fprintf(fp, "<data name=\"%s\">\n", name + 1);
		for (i = 0; i < level + 1; i++)
		{
			fprintf(fp, "\t");
		}

		fprintf(fp, "<struct>\n");	

		xml->current = node1;
		node2 = pub_xml_locnode(xml, "data");
		if (node2 == NULL)
		{
			pub_log_error("[%s][%d] [%s]Ϊstruct��δָ��data!", __FILE__, __LINE__, name + 1);
			xml->current = node_bak;
			return -1;
		}
		while (node2 != NULL)
		{
			xml->current = node2;
			if (pack_a(vars, fp, level + 2, xml) < 0)
			{
				pub_log_error("[%s][%d] ����struct[%s]ʧ��!", __FILE__, __LINE__, name + 1);
				xml->current = node_bak;
				return -1;
			}
			node2 = node2->next;
		}
		
		for (i = 0;i < level + 1; i++)
		{
			fprintf(fp, "\t");
		}
		
		fprintf(fp, "</struct>\n");	
		for (i = 0; i < level; i++)
		{
			fprintf(fp, "\t");
		}
		fprintf(fp, "</data>\n");	
	}
	else if ((node1 = pub_xml_locnode(xml, "array")) != NULL)
	{
		/*** array���� ***/
		node2 = pub_xml_locnode(xml, "name");
		if (node2 == NULL)
		{
			pub_log_error("[%s][%d] array��δָ��name!", __FILE__, __LINE__);
			xml->current = node_bak;
			return -1;
		}
		
		memset(name, 0x0, sizeof(name));
		name[0] = '#';
		strcpy(name + 1, node2->value);

		for (i = 0; i < level; i++)
		{
			fprintf(fp, "\t");
		}
		fprintf(fp, "<data name=\"%s\">\n", name + 1);
		for (i = 0; i < level + 1; i++)
		{
			fprintf(fp, "\t");
		}
		fprintf(fp, "<array>\n");	

		xml->current = node1;
		if ((node2 = pub_xml_locnode(xml, "data")) != NULL)
		{
			/****dataѭ��****/
			xml->current = node2;
			node3 = pub_xml_locnode(xml, "name");
			if (node3 == NULL)
			{
				pub_log_error("[%s][%d] data��δָ��name!", __FILE__, __LINE__);
				xml->current = node_bak;
				return -1;
			}
			
			memset(tmp, 0x0, sizeof(tmp));
			memset(value, 0x0, sizeof(value));
			sprintf(tmp, "#%s", node3->value);
			loc_get_zd_data(vars, tmp, value);
			pub_log_info("[%s][%d] ȡƽ̨����[%s]=[%s]", __FILE__, __LINE__, tmp, value);
			if (value[0] != '\0')
			{
				/***ѭ������data������ֵ,������ѭ������****/
				if (pack_a(vars, fp, level + 2, xml) < 0)
				{
					pub_log_info("[%s][%d] ����array[%s]ʧ��!", __FILE__, __LINE__, name + 1);
					xml->current = node_bak;
					return -1;
				}
			}
			else
			{
				/***ѭ������data������ֵ,���ļ���ȡѭ����***/
				FILE	*fpr = NULL;
				char	date[16];
				char	traceno[16];
				char	filename[128];

				memset(date, 0x0, sizeof(date));
				memset(traceno, 0x0, sizeof(traceno));
				memset(filename, 0x0, sizeof(filename));
				
				loc_get_zd_data(vars, "$date", date);
				loc_get_zd_data(vars, "$trace_no", traceno);
				sprintf(filename, "%s/dat/%s_%s_%s.dat", getenv("SWWORK"), date, traceno, name + 1);
				fpr = fopen(filename, "rb");
				if (fpr == NULL)
				{
					pub_log_info("[%s][%d�ļ�[%s]������!", __FILE__, __LINE__, filename);
				}
				else
				{
					/****��ʱtmp�л������ѭ�������������****/
					while (1)
					{
						memset(value, 0x0, sizeof(value));
						if (fgets(value, sizeof(value) - 1, fpr) == NULL)
						{
							break;
						}
						i = strlen(value) - 1;
						while (i > 0)
						{
							if (value[0] == ' '|| value [i] == ',' || value[i] == 0x0a || value[i] == 0x0d)
							{
								value[i] = '\0';
							}
							i--;
						}
						if (value[0] == '\0')
						{
							break;
						}
						loc_set_zd_data(vars, tmp, value);
						pub_log_info("[%s][%d] ȡ����[%s]=[%s]", __FILE__, __LINE__, tmp, value);
						if (pack_a(vars, fp, level + 2, xml) < 0)
						{
							pub_log_error("[%s][%d] ����array[%s]ʧ��!", __FILE__, __LINE__, name + 1);
							xml->current = node_bak;
							fclose(fpr);
							return -1;
						}
					}
					fclose(fpr);

					loc_set_zd_data(vars, tmp, "");
				}
			}
				
		}
		else if ((node2 = pub_xml_locnode(xml, "struct")) != NULL)
		{
			/****structѭ��****/
			xml->current = node2;
			node3 = pub_xml_locnode(xml, "data");
			if (node3 == NULL)
			{
				pub_log_error("[%s][%d] struct��δָ��data!", __FILE__, __LINE__);
				xml->current = node_bak;
				return -1;
			}
			xml->current=node3;
			node3 = pub_xml_locnode(xml, "name");
			if (node3 == NULL)
			{
				pub_log_error("[%s][%d] data��δָ��name!", __FILE__, __LINE__);
				xml->current = node_bak;
				return -1;
			}
			
			memset(tmp, 0x0, sizeof(tmp));
			sprintf(tmp, "#%s", node3->value);
			memset(value, 0x0, sizeof(value));
			loc_get_zd_data(vars, tmp, value);
			pub_log_info("[%s][%d] ȡƽ̨����[%s]=[%s]", __FILE__, __LINE__, tmp, value);
			if (value[0] != '\0')
			{
				/***ѭ������struct-data������ֵ,������ѭ������****/
				for (i = 0; i < level + 2; i++)
				{
					fprintf(fp, "\t");
				}
				fprintf(fp, "<struct>\n");	
				node4 = xml->current;
				while (node4 != NULL)
				{
					xml->current = node4;
					if (pack_a(vars, fp, level + 3, xml) < 0)
					{
						pub_log_error("[%s][%d] ����struct[%s]ʧ��!", __FILE__, __LINE__, name + 1);
						xml->current = node_bak;
						return -1;
					}
					node4 = node4->next;
				}
				for (i = 0; i < level + 2; i++)
				{
					fprintf(fp, "\t");
				}
				fprintf(fp, "</struct>\n");	
			}
			else
			{
				/***ѭ������struct-data������ֵ,���ļ���ȡѭ����***/
				FILE	*fpr = NULL;
				char	date[16];
				char	traceno[16];
				char	filename[128];
				sw_xmlnode_t	*node5 = NULL;
				
				memset(date, 0x0, sizeof(date));
				memset(traceno, 0x0, sizeof(traceno));
				memset(filename, 0x0, sizeof(filename));
			
				loc_get_zd_data(vars, "$date", date);
				loc_get_zd_data(vars, "$trace_no", traceno);
				sprintf(filename, "%s/dat/%s_%s_%s.dat", getenv("SWWORK"), date, traceno, name + 1);
				fpr = fopen(filename, "rb");
				if (fpr == NULL)
				{
					pub_log_info("[%s][%d] �ļ�[%s]������!", __FILE__, __LINE__, filename);
				}
				else
				{
					/****��ʱtmp�л������ѭ�������������****/	
					node3 = xml->current;
					while (1)
					{
						int	pos1 = 0;
						int	pos2 = 0;

						xml->current = node3;
						memset(value, 0x0, sizeof(value));
						if (fgets(value, sizeof(value) - 1, fpr) == NULL)
						{
							break;
						}
						for (i = 0; i < level + 2; i++)
						{
							fprintf(fp, "\t");
						}
						fprintf(fp, "<struct>\n");	
						node4 = xml->current;
						while (node4 != NULL)
						{
							xml->current = node4;
							node5 = pub_xml_locnode(xml, "name");
							if (node5 == NULL)
							{
								pub_log_error("[%s][%d] data��δָ��name!", __FILE__, __LINE__);
								fclose(fpr);
								xml->current = node_bak;
								return -1;
							}
							/*�Ѿ�ȡ��һ��������,���ڴ��ļ���ȡ��һ��������ֵ*/
							while (value[pos2] != '\0')
							{
								if (value[pos2] == ',' || value[pos2] == 0x0a || value[pos2] == 0x0d)
								{
									break;
								}
								pos2++;
							}
							if (pos2 == pos1)
							{
								/****��ǰ����������Ϊ��,��������****/
								if (value[pos2] != ',' && value[pos2] != 0x0a && value[pos2] != 0x0d)
								{
									/**����Ѿ�������β,֮�����б�����������**/
									pos1++;
									pos2++;
								}
							}
							else
							{
								/****�ѵ�ǰ�������****/
								char	tmp_value[1024];
								
								memset(tmp_value, 0x0, sizeof(tmp_value));
								memcpy(tmp_value, value + pos1, pos2 - pos1);
								loc_set_zd_data(vars, node5->value, tmp_value);
								pub_log_info("[%s][%d] ȡ����[%s]=[%s]", 
									__FILE__, __LINE__, node5->value, tmp_value);
								if (pack_a(vars, fp, level + 2, xml) < 0)
								{
									pub_log_error("[%s][%d] ����struct[%s]ʧ��!", __FILE__, __LINE__, name + 1);
									fclose(fpr);
									xml->current = node_bak;
									return -1;
								}
								loc_set_zd_data(vars, node5->value, "");
							}
							node4 = node4->next;
						}
						for (i = 0; i < level + 2; i++)
						{
							fprintf(fp, "\t");
						}
						fprintf(fp, "</struct>\n");	
					}
					fclose(fpr);
				}
			}
		}
		for (i = 0; i < level + 1; i++)
		{
			fprintf(fp, "\t");
		}
		fprintf(fp, "</array>\n");	
		for (i = 0; i < level; i++)
		{
			fprintf(fp, "\t");
		}
		fprintf(fp, "</data>\n");	
	}
	return 0;
}

/**
������:pack_jzcard
����  :������п���ʽ
����  :
       ���������	 
       ģ���ļ���
����ֵ:
       �������
˵��  :
**/ 
sw_int_t pack_jzcard(sw_loc_vars_t *vars, sw_buf_t *pkg_buf, sw_char_t *xmlname, int cache_flag)
{
	int	ret = 0;
	int	length = 0;
	FILE	*fp = NULL;
	char	buf[128];
	char	filename[256];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmltree_t	*xml = NULL;
	
	memset(buf, 0x0, sizeof(buf));
	memset(filename, 0x0, sizeof(filename));

	sprintf(filename, "%s/tmp/%d.xml", getenv("SWWORK"), getpid());
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] ���ļ�[%s]ʧ��! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fp, "<service>\n");
	fprintf(fp, "\t<sys-header>\n");
	
	if (cache_flag == 1)
	{
		xml = (sw_xmltree_t *)xmlname;
	}
	else
	{
		memset(buf, 0x0, sizeof(buf));
		sprintf(buf, "%s/cfg/common/%s", getenv("SWWORK"), xmlname);
		xml = pkg_read_xml(buf);
		if (xml == NULL)
		{
			pub_log_error("[%s][%d] Create xml tree error! xml=[%s]",
				__FILE__, __LINE__, buf);
			return -1;
		}
	}

	node1 = pub_xml_locnode(xml, ".service.sys-header.data");
	if (node1 == NULL)
	{
		pub_log_error("[%s][%d] ����ģ��δָ��sys-header!", __FILE__, __LINE__);
		fclose(fp);
		return -1;
	}

	if (pack_a(vars, fp, 2, xml) < 0)
	{
		pub_log_error("[%s][%d] ����sys-headerʧ��!", __FILE__, __LINE__);
		fclose(fp);
		return -1;
	}
	fprintf(fp, "\t</sys-header>\n");

	node1 = pub_xml_locnode(xml, ".service.app-header.data");
	if (node1 == NULL)
	{
		pub_log_info("[%s][%d] ����ģ��δָ��app-header!", __FILE__, __LINE__);
	}
	else
	{
		fprintf(fp, "\t<app-header>\n");
		if (pack_a(vars, fp, 2, xml) < 0)
		{
			pub_log_error("[%s][%d] ����sys-headerʧ��!", __FILE__, __LINE__);
			fclose(fp);
			return -1;
		}
		fprintf(fp, "\t</app-header>\n");
	}

	node1 = pub_xml_locnode(xml, ".service.local-header.data");
	if (node1 == NULL)
	{
		pub_log_info("[%s][%d] ����ģ��δָ��local-header!", __FILE__, __LINE__);
	}
	else
	{
		fprintf(fp, "\t<local-header>\n");
		if (pack_a(vars, fp, 2, xml) < 0)
		{
			pub_log_error("[%s][%d] ����local-headerʧ��!", __FILE__, __LINE__);
			fclose(fp);
			return -1;
		}
		fprintf(fp, "\t</local-header>\n");
	}

	node1 = pub_xml_locnode(xml, ".service.body");
	/****���ҷ���MESSAGE_TYPE��MESSAGE_CODE������body****/
	while (node1 != NULL)
	{
		xml->current = node1;
		node2 = pub_xml_locnode(xml, "MESSAGE_TYPE");
		if (node2 != NULL)
		{
			char	msgtype[32];
			
			memset(msgtype, 0x0, sizeof(msgtype));
			
			loc_get_zd_data(vars, "#MESSAGE_TYPE", msgtype);
			if (msgtype[0] == '\0')
			{
				/****δָ��#MESSAGE_TYPE,����������һ��body**/
				node1 = node1->next;
				continue;
			}
			if (strstr(node2->value, msgtype) == NULL)
			{
				/****MESSAGE_TYPE����������,����������һ��body**/
				node1 = node1->next;
				continue;
			}
		}

		node2 = pub_xml_locnode(xml, "MESSAGE_CODE");
		if (node2 != NULL)
		{
			char	msgcode[32];

			memset(msgcode, 0x0, sizeof(msgcode));
			loc_get_zd_data(vars, "#MESSAGE_CODE", msgcode);
			if (msgcode[0] == '\0')
			{
				/****δָ��#MESSAGE_CODE,����������һ��body**/
				node1 = node1->next;
				continue;
			}
			if (strstr(node2->value, msgcode) == NULL)
			{
				/****MESSAGE_CODE����������,����������һ��body**/
				node1 = node1->next;
				continue;
			}
		}
		break;
	}

	if (node1 == NULL)
	{
		pub_log_info("[%s][%d] ����ģ��δָ��body!", __FILE__, __LINE__);
	}
	else
	{
		fprintf(fp, "\t<body>\n");
		xml->current = node1;
		node1 = pub_xml_locnode(xml, "data");
		while (node1 != NULL)
		{
			xml->current = node1;
			if (pack_a(vars, fp, 2, xml)<0)
			{
				pub_log_error("[%s][%d] ����bodyʧ��!", __FILE__, __LINE__);
				fclose(fp);
				return -1;
			}
			node1 = node1->next;
		}
		fprintf(fp, "\t</body>\n");
	}
	fprintf(fp, "</service>\n");
	fclose(fp);
	
	/*** ������ı���д�뻺���� ***/
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		pub_log_error("[%s][%d] ���´��ļ�[%s]ʧ��! errno=[%d]:[%s]",
			__FILE__, __LINE__, filename, errno, strerror(errno));
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	ret = pub_buf_chksize(pkg_buf, length);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] check buf size error! buf->size=[%d] buf->len=[%d] len=[%d]",
			__FILE__, __LINE__, pkg_buf->size, pkg_buf->len, length);
		return -1;
	}
	sprintf(pkg_buf->data + pkg_buf->len, "%08d", length);
	pkg_buf->len += 8;
	fread(pkg_buf->data + pkg_buf->len, length, 1, fp);
	pkg_buf->len += length;
	pkg_buf->data[pkg_buf->len] = '\0';
	fclose(fp);
	
	return pkg_buf->len;
}

/**
������:unpack_a
����  :������п���ʽ
����  :
       xml���ݽṹ
����ֵ:
       0/-1
˵��  :
		1.field��ֱ�Ӱ�����ת��
		2.struct��ֱ�ӽ������е�����
		3.arrayֻ����һ��
**/
int unpack_a(sw_loc_vars_t *vars, sw_xmltree_t *xml, int cnt)
{
	int 	index = 0;
	char	name[64];
	char	tmp[128];
	sw_xmlnode_t	*node1 = NULL;
	sw_xmlnode_t	*node2 = NULL;
	sw_xmlnode_t	*node3 = NULL;
	sw_xmlnode_t	*node_bak = xml->current;
	
	memset(name, 0x0, sizeof(name));
	
	node1 = xml->current;
	if ((node1 = pub_xml_locnode(xml, "field")) != NULL)
	{
		/****��:field***/
		node2 = pub_xml_locnode(xml, "name");
		node3 = pub_xml_locnode(xml, "field");
		if (node2 != NULL && node3 != NULL && node2->value != NULL && node3->value!=NULL)
		{
			memset(name, 0x0, sizeof(name));
			name[0] = '#';
			strcpy(name + 1, node2->value);
			if (cnt >= 0)
			{
				memset(tmp, 0x0, sizeof(tmp));
				sprintf(tmp, "(%d)", cnt);
				strcat(name, tmp);
			}
			loc_set_zd_data(vars, name, node3->value);
			pub_log_info("[%s][%d] �������[%s]=[%s]", __FILE__, __LINE__, name, node3->value);
		}
	}
	else if ((node1 = pub_xml_locnode(xml, "struct")) != NULL)
	{
		node2 = pub_xml_locnode(xml, "struct.data");
		while (node2 != NULL)
		{
			xml->current = node2;
			if (unpack_a(vars, xml, -1) < 0)
			{
				pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
				xml->current = node_bak;
				return -1;
			}
			node2 = node2->next;
		}
	}
	else if ((node1 = pub_xml_locnode(xml, "array")) != NULL)
	{
		index = 0;
		node2 = pub_xml_locnode(xml, "array.struct");
		while (node2 != NULL)
		{
			index++;
			node2 = node2->next;
		}
		
		if (index == 1)
		{
			node2 = pub_xml_locnode(xml, "array.struct.data");	
			while (node2 != NULL)
			{
				xml->current = node2;
				if (unpack_a(vars, xml, -1) < 0)
				{
					pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
					xml->current = node_bak;
					return -1;
				}
				node2 = node2->next;
			}
		}
		else if (index > 1)
		{
			index = 0;
			node2 = pub_xml_locnode(xml, "array.struct");
			while (node2 != NULL)
			{
				xml->current = node2;
				node3 = pub_xml_locnode(xml, "data");	
				while (node3 != NULL)
				{
					xml->current = node3;
					if (unpack_a(vars, xml, index) < 0)
					{
						pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
						xml->current = node_bak;
						return -1;
					}
					node3 = node3->next;
				}
				index++;
				node2 = node2->next;
			}
		}
		
		node2 = pub_xml_locnode(xml, "array.data");	
		if (node2 != NULL)
		{
			xml->current = node2;
			if (unpack_a(vars, xml, -1) < 0)
			{
				pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
				xml->current = node_bak;
				return -1;
			}
		}
	}
	xml->current = node_bak;

	return 0;
}

/**
������:unpack_jzcard
����:������п���ʽ
����:
	���뻺����	 
	ģ���ļ���(��ʵ����)
����ֵ:
       0/-1 
˵��:
	1.field��ֱ�Ӱ�����ת��
	2.struct��ֱ�ӽ������е�����
	3.arrayֻ����һ��
**/ 
sw_int_t unpack_jzcard(sw_loc_vars_t *vars, sw_char_t *pkg, sw_char_t *xmlname, int len, int cache_flag)
{
	sw_xmltree_t *xml = NULL;
	sw_xmlnode_t *node1 = NULL;
	
	xml = pub_xml_crtree_ext(pkg, len);
	if (xml == NULL)
	{
		pub_log_error("[%s][%d] create xml tree error!", __FILE__, __LINE__);
		pub_log_bin(SW_LOG_ERROR, pkg, len, "[%s][%d] create xml tree error!", __FILE__, __LINE__);
		return -1;
	}
	
	/****��:sys-header****/
	node1 = pub_xml_locnode(xml,".service.sys-header.data.struct.data");
	while (node1 != NULL)
	{
		xml->current = node1;
		if (unpack_a(vars, xml, -1) < 0)
		{
			pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		node1 = node1->next;
	}

	/****��:app-header****/
	node1 = pub_xml_locnode(xml, ".service.app-header.data.struct.data");
	while (node1 != NULL)
	{
		xml->current = node1;
		if (unpack_a(vars, xml, -1) < 0)
		{
			pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		node1 = node1->next;
	}

	/****��:local-header****/
	node1 = pub_xml_locnode(xml, ".service.local-header.data.struct.data");
	while (node1 != NULL)
	{
		xml->current = node1;
		if (unpack_a(vars, xml, -1) < 0)
		{
			pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		node1 = node1->next;
	}

	/****��:body****/
	node1 = pub_xml_locnode(xml, ".service.body.data");
	while (node1 != NULL)
	{
		xml->current = node1;
		if (unpack_a(vars, xml, -1) < 0)
		{
			pub_log_error("[%s][%d] unpack_a error!", __FILE__, __LINE__);
			pub_xml_deltree(xml);
			return -1;
		}
		node1 = node1->next;
	}
	pub_xml_deltree(xml);

	return 0;
}

