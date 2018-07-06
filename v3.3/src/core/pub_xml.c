/*********************************************************************
 *** �� �� ��: v2.0
 *** ��������: Shi Shenghui
 *** ��������: 2008-01-29
 *** ����ģ��: ����ģ��
 *** ��������: pub_xml.c
 *** ��������: xml��ʽ�ļ�����������,xml������
 *** �����б�:
 ***		xml_initnode		���ڵ��ʼ��
 ***		pub_xml_readline		��ȡһ������
 ***		pub_xml_line_getvalue		��һxml���еõ�ֵ���֣�������ֵ֮����һ���ַ���λ��
 ***		pub_xml_line_getname		��һ���еõ����Ʋ��֣�����������֮����һ���ַ���λ��
 ***		pub_xml_line_getattr		��һxml���еõ����Բ��֣�����������֮����һ���ַ���λ��	
 ***		pub_xml_setcurrent		��������ڵ�Ϊ��ǰxml���ڵ�
 ***		pub_xml_prt			��ӡ��ǰxml���ڵ㼰����ȫ���ڵ�
 ***		pub_xml_locnode			��λĳ�����
 ***		pub_xml_crtree			����
 ***		pub_xml_crnode			����ָ����ǩ��֧�ڵ�
 ***		pub_xml_dealvalue		����ǰֵ
 ***		pub_xml_delnode			�����ǰ�ڵ�
 ***		pub_xml_unpack			�齨xml��
 ***		pub_xml_pack			��xml�����ļ�
 ***		xml_savenode		���浱ǰ�ڵ㵽�ļ�,�ݹ麯��
 ***		pub_xml_get_nodevalue		�õ���ǰ��ǩ���ӱ�ǩ�����Ի��ӱ�ǩ��ֵ
 *** ʹ��ע��:
 *** �޸ļ�¼:
 *** 	�޸�����:liteng
 *** 	�޸�ʱ��:2011-11-29
 *** 	�޸�����:�����˴����ʽ�����¶����˺����ӿ�
 ********************************************************************/

#include "pub_xml.h"
#include "pub_buf.h"

#if !defined(__XML_STACK__)

#define CHECKPTR(p) if (p == NULL) \
		{\
			pub_log_error("[%s][%d],allocted mem error,[%d]",__FILE__,__LINE__,errno);\
			goto ERR_EXIT;\
		}

static sw_int_t xmlmem_init(sw_xmlmem_t *ptr,char *s,int type);
static sw_int_t xmlmem_clear(sw_xmlmem_t *sf,int type);
static sw_int_t xmlmem_read(char *buffer,int items,int size,sw_xmlmem_t *sf,int type);
static sw_int_t xmlmem_write(sw_xmlmem_t *sf,int type, const char *fmt,...);
static sw_int_t xml_strcpy(char *s1,char *s2);
static sw_int_t xml_initnode(sw_xmlnode_t *node);
static sw_int_t xml_readline(sw_xmlmem_t *fp,sw_buf_t *pbuf,char *readbuf,int *pos,int type);
static sw_int_t xml_readline_a(sw_xmlmem_t *fp,sw_buf_t *pbuf,char *readbuf,int *pos,int type);
static sw_int_t xmlline_getvalue(char *line, sw_buf_t *vbuf);
static sw_int_t xmlline_getname(char *line,char *name,int iPos);
static sw_int_t xmlline_getattr(char *line,char *pattrname,char *psAttrValue,int iPos);
static char *xml_dealvalue(char *value);
static sw_int_t xml_savenode(sw_xmlnode_t *pstCurNode,int iLevel,sw_xmlmem_t *fpt,int type);
static sw_xmltree_t * xml_crtree(sw_xmlmem_t *fp,int type);

int	g_xmlalloc = 0;
char	*g_xmlvalue = NULL;
char	g_xmlbuf[ITEM_VALUE_LEN * 4 + 1];


static sw_int_t xmlmem_init(sw_xmlmem_t *ptr,char *s,int type)
{
	pub_mem_memset(ptr,0x00,sizeof(sw_xmlmem_t));
	if (type == XML_MEM_TYPE)
	{
		ptr->start=s;
		ptr->current=s;
		ptr->length=0;	
	}
	else if (type == XML_FILE_TYPE)
	{
		ptr->fp = NULL;
		ptr->fp= fopen(s,"rb");
		if (ptr->fp  == NULL)
		{
			pub_log_error("[%s][%d],pubxmlcrtee open error,[%d][%s]",__FILE__,__LINE__,errno,s);
			return SW_ERR;
		}
	}
	else
	{
		return SW_ERR;
	}

	return SW_OK;
}

static sw_int_t xmlmem_init_ext(sw_xmlmem_t *ptr, char *s,int type)
{
	pub_mem_memset(ptr,0x00,sizeof(sw_xmlmem_t));
	if (type == XML_MEM_TYPE)
	{
		ptr->start=s;
		ptr->current=s;
		ptr->length=0;	
	}
	else if (type == XML_FILE_TYPE)
	{
		ptr->fp = NULL;
		ptr->fp= fopen(s,"wb");
		if (ptr->fp  == NULL)
		{
			pub_log_error("[%s][%d],pubxmlcrtee open error,[%d][%s]",__FILE__,__LINE__,errno,s);
			return SW_ERR;
		}
	}
	else
	{
		return SW_ERR;
	}

	return SW_OK;
}

static sw_int_t xmlmem_clear(sw_xmlmem_t *sf,int type)
{
	if (type == XML_FILE_TYPE)
	{
		fclose(sf->fp);
	}
	return SW_OK;
}
 
static sw_int_t xmlmem_read(char *buffer,int items,int size,sw_xmlmem_t *sf,int type)
{
	if (type == XML_MEM_TYPE)
	{
		int nTry=items*size;
		if(nTry>sf->length-(sf->current-sf->start)){
			nTry=sf->length-(sf->current-sf->start);
		}
		if(nTry<=0){
			return(0);
		}
		memcpy(buffer,sf->current,nTry);
		sf->current+=nTry;
		return(nTry);
	}
	else if (type == XML_FILE_TYPE)
	{
		return fread(buffer,items,size,sf->fp);
	}
	
	return SW_ERROR;
 }
 
 static sw_int_t xmlmem_write(sw_xmlmem_t *sf, int type, const char *fmt,...)
 {
	va_list		args;
	va_start(args, fmt);
	sw_int32_t	step = 0;
	
	if (type == XML_MEM_TYPE)
	{
		step += vsprintf(sf->current, fmt, args);
		if (step < 0)
		{
			return SW_ERR;
		}
		else
		{
			sf->length += step;
			sf->current = sf->start + sf->length;
		}
		
	}
	else if (type == XML_FILE_TYPE)
	{
		vfprintf(sf->fp, fmt, args);
	}
	else
	{
		return SW_ERR;
	}
	va_end(args);

	return SW_OK;
 }
 
 /****ת����XML�ؼ��ֵ��ַ�������****/
 static sw_int_t xml_strcpy(char *s1,char *s2)
 {
	int i=0;
	int j=0;
	while (s2[i]!='\0')
	{
		if (s2[i] == '>')
		{
			strcpy(s1 + j, "&gt;");
			j+=4;
			i++;
		}
		else if (s2[i] == '<' )
		{
			strcpy(s1+j,"&lt;");
			j+=4;
			i++;
		}
		else if (s2[i] == '&')
		{
			strcpy(s1+j,"&amp;");
			j+=5;
			i++;
		}
		else if (s2[i] == '\'')
		{
			strcpy(s1+j,"&apos;");
			j+=6;
			i++;                                                                                                      
		}                                                                                                                  
		else if (s2[i] == '"')                                                                                             
		{                                                                                                                  
			strcpy(s1+j,"&quot;");                                                                                    
			j+=6;                                                                                                     
			i++;                                                                                                      
		}                                                                                                                  
		else                                                                                                               
		{                                                                                                                  
			s1[j++]=s2[i++];                                                                                           
		}
	}
	s1[j]='\0';

	return(SW_OK);	 
 }

 static sw_int_t xml_strlen(char *s)
 {
	int	i = 0;
	int	j = 0;

	while (s[i] != '\0')
	{
		if (s[i] == '>')
		{
			j += 4;
			i++;
		}
		else if (s[i] == '<' )
		{
			j += 4;
			i++;
		}
		else if (s[i] == '&')
		{
			j += 5;
			i++;
		}
		else if (s[i] == '\'')
		{
			j += 6;
			i++;                                                                                                      
		}                                                                                                                  
		else if (s[i] == '"')                                                                                             
		{                                                                                                                  
			j += 6;                                                                                                     
			i++;                                                                                                      
		}                                                                                                                  
		else                                                                                                               
		{                                                                                                                  
			j++;
			i++;                                                                                           
		}
	}

	return j;
}

/******************************************************************************
 *** ��������: xml_initnode
 *** ��������: ���ڵ��ʼ��
 *** ��������: Shi Shenghui
 *** ��������: 2008-01-29 15:32
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: pstXmlNode		���ڵ�ָ��
 *** �������: ��
 *** �� �� ֵ: void
 *** ע������: 
 ******************************************************************************/
static sw_int_t xml_initnode(sw_xmlnode_t *node)
{
	node->next = NULL;
	node->firstchild = NULL;
	node->name = NULL;
	node->value = NULL;
	node->parent = NULL;
	node->node_type = 0;
	return SW_OK;
}

/******************************************************************************
 *** ��������: xml_readline
 *** ��������: ��ȡһ������
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 15:36
 *** ���ú���: 
 *** 	����1: xml_readline_a
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: fp		�ļ�ָ��
 *** 	����2: line		��ǰ������ָ��
 *** 	����3: readbuf	���ݻ�����
 *** 	����4: pos	��һ��λ��ָ��
 *** �������: ��
 *** �� �� ֵ: void
 *** ע������: 
 ******************************************************************************/
static sw_int_t xml_readline(sw_xmlmem_t *fp, sw_buf_t *buf, char *readbuf,int *pos,int type)
{
	int	idx = 1;
	char	*line = NULL;
	/*��һ��*/	
	while(1)
	{
		buf_refresh(buf);
		xml_readline_a(fp, buf, readbuf, pos, type);
		line = buf->data;
		if (line[0] == '<')
		{
			idx = 1;
			while(line[idx] != '\0')
			{
				if (line[idx] == ' ')
				{
					return SW_OK;
				}
				else if (line[idx] == '>')
				{
					return SW_OK;
				}
			#ifndef XML_PACK_NULL
				else if ((line[idx] == '/')&&(line[idx+1] == '>'))
				{
					break;
				}
			#endif
				else
				{
					idx++;
				}
			}
			if (line[idx] == '\0')
			{
				return SW_OK;
			}
			else
			{
				line[0] = '\0';
				continue;
			}
		}
		else
		{
			return SW_OK;
		}
	}
}


/******************************************************************************
 *** ��������: xml_readline_a
 *** ��������: ��ȡһ��������չ����
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 15:36
 *** ���ú���: 
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: fp		�ļ�ָ��
 *** 	����2: line		��ǰ������ָ��
 *** 	����3: readbuf	���ݻ�����
 *** 	����4: pos	��һ��λ��ָ��
 *** �������: ��
 *** �� �� ֵ: void
 *** ע������: 
  ******************************************************************************/
static sw_int_t xml_readline_a(sw_xmlmem_t *fp, sw_buf_t *buf, char *readbuf,int *pos,int type)
{	/*��һ��*/
	int	pos1;
	int	bakpos;		/*����֮ǰ��iPos1λ�ã���ע�������ָ�*/
	int	readline;
	char	cChar;
	char	cBak;
	char	cTmp;
	char	cCh;
	int	iBinQuot;
	int	docflag = 0;
	char	*line = NULL;
	
	/*�ڶ������ж�����һ��<>�������Ĳ���,����������Ҫ���¶��ļ�*/
	line = buf->data;
	line[0] = '\0';
	pos1 = 0;
	iBinQuot = 0;
	while(1)
	{
		line = buf->data;
		cChar = readbuf[(*pos)++];
		if (cChar == '\0')
		{
			memset(readbuf, 0x0, XML_READ_BUF_MAX);
			readline = xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
			(*pos) = 0;
			readbuf[readline] = '\0';
			if (readline == 0)
			{	/*��Ҫ�����������*/
				line[0] = '\0';
				break;
			}
			cChar = readbuf[(*pos)++];
		}
		if (!iBinQuot)
		{	/*��˫������Ĳ��֣������пհ׶�����һ���ո�*/
			if ((cChar == 13 || cChar == 10 || cChar == '\t') && (pos1 == 0 || line[0] == '<'))
			{
				cChar = ' ';
			}
			if (cChar == ' ')
			{	/*�Ⱥ�֮��Ҳ��Ҫ�ո�*/
				if (pos1 == 0 || (line[pos1-1] == ' ' && line[0] == '<'))
				{
					continue;
				}
				
				/*** ֻɾ�������еȺ����˵Ŀո� ***/
				if (line[pos1 - 1] == '=' && line[0] == '<')
				{
					continue;
				}
			}
			if (cChar == '"' && (line[0] == '<' || pos1 == 1))/*��˫������ı����ű�־*/
			{
				iBinQuot = 1;
			}
			/*** ֻɾ�������еȺ����˵Ŀո� ***/
			if ((cChar == '=' && line[0] == '<') || (cChar == '/' && line[0] == '<')) /*��Ϊ�Ⱥţ���ɾ��֮ǰ�����пո�*/
			{
				if ((pos1>0)&&(line[pos1-1] == ' '))
				{
					pos1--;
				}
			}
			if (cChar == '-')
			{	/*ɾ��ע��*/
				cBak = '\0';
				cTmp = '\0';
				cCh = '\0';
				/*printf("Line=%s\n",line);*/
				if ((pos1 > 2)&&(line[pos1-1] == '-')
				&&(line[pos1-2] == '!')&&(line[pos1-3] == '<'))
				{
					bakpos = pos1 - 3;
					do
					{
						if ((cTmp == '-')&&(cCh == '-'))
						{
							cBak = cTmp;
							cTmp = cCh;
						}
						else
						{
							if (cCh == '-')
							{
								cBak = cCh;
							}
							else
							{
								cChar = readbuf[(*pos)++];
								if (cChar == '\0')
								{
									readline= xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
									(*pos) = 0;
									readbuf[readline] = '\0';
									cChar = readbuf[(*pos)++];
								}
								cBak = cChar;
							}
							cChar = readbuf[(*pos)++];
							if (cChar == '\0')
							{
								readline = xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
								(*pos) = 0;
								readbuf[readline] = '\0';
								cChar = readbuf[(*pos)++];
							}
							cTmp = cChar;
						}
						cChar = readbuf[(*pos)++];
						if (cChar == '\0')
						{
							readline = xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
							(*pos) = 0 ;
							readbuf[readline] = '\0';
							cChar = readbuf[(*pos)++];
						}
						cCh = cChar;
					}while((cBak != '-')||(cTmp != '-')||(cCh != '>'));
					pos1 = bakpos;
					docflag = 0;
					continue;
				}/* end if iPos1>1 && line....*/
			}/* end if cChar=='-'*/
			if (cChar == '?')
			{	
				/*ɾ��?ע��*/
				if ((pos1 > 0)&&(line[pos1-1] == '<'))
				{
					cBak = '\0';
					cTmp = '\0';
					do
					{
						if (cTmp == '?')
						{
							cBak = cTmp;
						}
						else
						{
							cChar = readbuf[(*pos)++];
							if (cChar == '\0')
							{
								readline = xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
								(*pos) = 0;
								readbuf[readline] = '\0';
								cChar = readbuf[(*pos)++];
							}
							cBak = cChar;
						}
						cChar = readbuf[(*pos)++];
						if (cChar == '\0')
						{
							readline = xmlmem_read(readbuf,1,XML_READ_BUF_MAX - 1,fp,type);
							(*pos) = 0;
							readbuf[readline] = '\0';
							cChar = readbuf[(*pos)++];
						}
						cTmp = cChar;
					}while((cBak != '?')||(cTmp != '>'));
					pos1 = 0;
					docflag = 0;
					continue;
				}/* end if iPos1 > 0 && line....*/
			}/* end if cChar == '?'*/
	
			/*** <! DOCTYPE > ***/
			if (cChar == '!' && pos1 > 0 && line[pos1 - 1] == '<' && readbuf[*pos] != '-')
			{
				docflag = 1;
			}

			/*��������д��psLine���ַ�*/
			buf->len = pos1;
			buf_format_append(buf, "%c", cChar);
			pos1++;
			if (cChar == '>')
			{
				if (docflag == 0)
				{
					line[pos1] = '\0';
					break;
				}
				else
				{
					docflag = 0;
				}
			}
		}
		else
		{	
			/*else binqot */
			buf->len = pos1;
			buf_format_append(buf, "%c", cChar);
			pos1++;
			if (cChar == '"')/*��˫������ı����ű�־*/
			{
				iBinQuot = 0;
			}
			
		}/* end if iBinQuot condition */
	}/* end while */

	return SW_OK;
}

/******************************************************************************
 *** ��������: pub_xml_line_getvalue
 *** ��������: ��һxml���еõ�value���֣�������value֮����һ���ַ���λ��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 15:42
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: line		��ǰ�����ݻ�����
 *** �������: ��
 *** 	����1: pvalue		��ǰ��ֵ����
 *** �� �� ֵ: value֮����һ���ַ���λ��
 *** ע������: 
 ******************************************************************************/

static sw_int_t xmlline_getvalue(char *line, sw_buf_t *vbuf)
{
	int	iPos = 0;
	int	iCurPos = 0;
	char	*value = NULL;

	iPos = 0;
	while(line[iPos] != '\0')
	{
		if (line[iPos] != '<' || (line[iPos] == '<' && line[iPos + 1] == '!'))
		{
			iPos++;
		}
		else
		{
			break;
		}
	}
	
	value = vbuf->data;
	strcpy(value,"");
	if (iPos != 0)
	{
		buf_update_string(vbuf, line, iPos);
		value = vbuf->data;
		value[iPos] = '\0';
		iCurPos = iPos;
		while(value[iCurPos-1] == ' ')
		{
			iCurPos--;
			if (iCurPos-1 < 0)
			{
				break;
			}
		}
		value[iCurPos] = '\0';
	}
	return iPos;
}


/******************************************************************************
 *** ��������: pub_xml_line_getname
 *** ��������: ��һxml���еõ����Ʋ��֣�����������֮����һ���ַ���λ��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 15:47
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: line		��ǰ�����ݻ�����
 ***	����2: iPos		��ǰλ��
 *** �������: 
 ***    ����1: name		��ǰ�����Ʋ���
 *** �� �� ֵ: ��������֮����һ���ַ���λ��
 *** ע������: 
 ******************************************************************************/
static sw_int_t xmlline_getname(char *line,char *name,int iPos)
{
	int iPos1;
	
	iPos1 = iPos + 1;
	iPos1++;
	if (line[iPos1] == '/')
	{
		while(line[iPos1] != '\0' && line[iPos1] != '>')
		{
			iPos1++;
		}

		memcpy(name,line + iPos + 1,iPos1 - iPos - 1);
		name[iPos1-iPos-1] = '\0';
		return iPos1;
	}
	else
	{
#ifdef XML_PACK_NULL
		while(line[iPos1] != '\0' && line[iPos1] != ' '&& line[iPos1] != '>' && strncmp(line + iPos1, "/>", 2) != 0)
		{
			iPos1++;
		}
#else
		while (line[iPos1] != '\0' && line[iPos1] != ' ' && line[iPos1] != '>')
		{
			iPos1++;
		}
#endif
		memcpy(name,line + iPos + 1,iPos1 - iPos - 1);
		name[iPos1-iPos-1] = '\0';
		return iPos1;
	}
	return SW_ERR;
}

/******************************************************************************
 *** ��������: pub_xml_line_getattr
 *** ��������: ��һxml���еõ�attrib���֣�������attrib֮����һ���ַ���λ��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 15:52
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: line		��ǰ�л�����
 ***	����2: iPos		��ǰλ��
 *** �������: 
 *** 	����1: pattrname	����������
 ***	����2: psAttrValue	����ֵ����
 *** �� �� ֵ: ��������֮����һ���ַ���λ��
 *** ע������: 
 ******************************************************************************/
static sw_int_t xmlline_getattr(char *line,char *pattrname,char *psAttrValue,int iPos)
{
	int iPos1;
	int iPos2;

	while(line[iPos] == ' ')
	{
		iPos++;
	}

	if ((line[iPos] == '>')||((line[iPos] == '/')&&(line[iPos+1] == '>')))
	{
		strcpy(pattrname,"");
		strcpy(psAttrValue,"");
		if (line[iPos] == '>')
		{
			return SW_S_NOTALL_END;
		}
		else
		{
			return SW_S_ALL_END;
		}
	}

	iPos--;
	iPos1 = iPos + 1;
	
	/*while(line[iPos1]!=' ' && (line[iPos1]!='/' || line[iPos1+1]!='>')){
		iPos1++;
	}*/
	while(line[iPos1] != '\0' && line[iPos1] != '"')
	{
		iPos1++;
	}

	iPos1++;
	while( line[iPos1] != '\0' && line[iPos1] != '"')
	{
		iPos1++;
	}
	iPos1++;

	iPos2 = iPos + 1;/*����=λ��*/
	while(line[iPos2] != '\0' && line[iPos2] != '=')
	{
		iPos2++;
	}
	
	/*��д���Ե�����*/
	memcpy(pattrname,line + iPos + 1,iPos2 - iPos - 1);
	pattrname[iPos2-iPos-1] = '\0';
	
	/*��д���Ե�ֵ*/
	iPos2++;
	memcpy(psAttrValue,line + iPos2 + 1,iPos1 - iPos2 - 2);
	psAttrValue[iPos1-iPos2-2] = '\0';

	/*printf("iPos1=%d,iPos2=%d\n",iPos1,iPos2);
	printf("pattrname:%s,psAttrValue:%s\n",pattrname,psAttrValue);
	printf("line=%s\n",line);
	printf("iPos1=%d\n",iPos1);	
	getchar();*/

	return iPos1;
}

/******************************************************************************
 *** ��������: xml_dealvalue
 *** ��������: ����ǰֵ
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 16:39
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: psValue		��ǰҪ�����ֵָ��
 *** �������: ��
 *** �� �� ֵ: ������
 *** ע������: 
 ******************************************************************************/
static char * xml_dealvalue(char *value)
{
	int idx;
	int j;
	int iLen;

	/*ɾ��β���ո�˫����*/
	idx = strlen(value) - 1;
	/*** ����ɾ���ڵ�β���Ŀո� ***/
#if 0
	while((value[idx] == ' ')&&(idx >= 0))
	{
		value[idx] = '\0';
		idx--;
	}
#endif

	if (idx < 0) 
	{
		return NULL;
	}

	if (value[0] == '"' && value[idx] == '"')
	{
		value[idx] = '\0';
		idx = 0;
		while((value[idx] != '"')&&(value[idx] != '\0'))
		{
			idx++;
		}

		if (value[idx] == '"')
		{
			j = 0;
			idx++;
			while(value[idx] != '\0')
			{
				value[j] = value[idx];
				idx++;
				j++;
			}
			value[j] = '\0';
		}
	}
	
	/*�����ַ� < > "�Ĵ���"*/
	idx = 0;
	iLen = strlen(value);
	while(value[idx] != '\0')
	{
		if (value[idx] == '&')
		{
			/******************************* 
				&lt;    <    С��
				&gt;	>    ����
				&amp;   &    �ͺ�
				&apos;  '    ������
				&quot;  "    ����
			*******************************/
			if (strncmp(value + idx, "&lt;", 4) == 0)
			{
				j = idx + 1;
				value[idx] = '<';
				while (value[j] != '\0')
				{
					value[j] = value[j+3];
					j++;
				}
				value[j] = '\0';
				iLen -= 3;
				idx++;
			}
			else if (strncmp(value + idx, "&gt;", 4) == 0)
			{
				j = idx + 1;
				value[idx] = '>';
				while (value[j] != '\0')
				{
					value[j] = value[j+3];
					j++;
				}
				value[j] = '\0';
				iLen -= 3;
				idx++;
			}
			else if (strncmp(value + idx, "&amp;", 5) == 0)
			{
				j = idx + 1;
				value[idx] = '&';
				while (value[j] != '\0')
				{
					value[j] = value[j+4];
					j++;
				}
				value[j] = '\0';
				iLen -= 4;
				idx++;
			}
			else if (strncmp(value + idx, "&apos;", 6) == 0)
			{
				j = idx + 1;
				value[idx] = '\'';
				while (value[j] != '\0')
				{
					value[j] = value[j+5];
					j++;
				}
				value[j] = '\0';
				iLen -= 5;
				idx++;
			}
			else if (strncmp(value + idx, "&quot;", 6) == 0)
			{
				j = idx + 1;
				value[idx] = '\"';
				while (value[j] != '\0')
				{
					value[j] = value[j+5];
					j++;
				}
				value[j] = '\0';
				iLen -= 5;
				idx++;
			}
			else
			{
				idx++;
			}
		}
		else	/*end if*/
		{
			idx++;
		}
	}
	return value;
}

static int xmlbuf_cpy(char *value)
{
	int	len = 0;
	
	g_xmlalloc = 0;
	memset(g_xmlbuf, 0x0, sizeof(g_xmlbuf));
	g_xmlvalue = g_xmlbuf;
	
	len = strlen(value);
	if (len > ITEM_VALUE_LEN)
	{
		g_xmlvalue = (char *)calloc(1, len * 4 + 1);
		if (g_xmlvalue == NULL)
		{
			pub_log_error("[%s][%d] Calloc error, size=[%d] errno=[%d]:[%s]",
				__FILE__, __LINE__, len * 4, errno, strerror(errno));
			return -1;
		}
		g_xmlalloc = 1;
	}
	xml_strcpy(g_xmlvalue, value);
	
	return 0;
}

static int xmlbuf_free()
{
	if (g_xmlalloc == 1)
	{
		free(g_xmlvalue);
		g_xmlvalue = g_xmlbuf;
		g_xmlalloc = 0;
	}
	
	return 0;
}

/******************************************************************************
 *** ��������: xml_savenode
 *** ��������: ���浱ǰ�ڵ㵽�ļ�,�ݹ麯��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 18:39
 *** ���ú���: 
 *** 	����1: xml_savenode
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: pstCurNode		��ǰ�ڵ�ָ��
 *** 	����2: iLevel			��ǰ�ڵ��μ���
 *** 	����3: pstCurNode		�ļ�ָ��
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
static sw_int_t xml_savenode(sw_xmlnode_t *pstCurNode,int iLevel,sw_xmlmem_t *fpt,int type)
{
	int	have_attr = 0;
	sw_int32_t	i = 0;
	sw_xmlnode_t	*pstNode1 = NULL;
	sw_xmlnode_t	*pstNode2 = NULL;
	sw_int_t	result = SW_ERR;

	if (pstCurNode == NULL)
	{
		return SW_ERR;
	}

	if (pstCurNode->node_type == SW_NODE_ATTRIB)
	{
		/*���������������ڵ�*/
		return SW_ERR;
	}

	pstNode1 = pstCurNode;
	if (iLevel > 0) 
	{
		for(i = 0;i < iLevel;i++)
		{
			result = xmlmem_write(fpt, type,"\t");
			if (result != SW_OK)
			{
				return SW_ERR;
			}
		}
	}
	
	result = xmlmem_write(fpt, type, "<%s", pstNode1->name);
	if (result != SW_OK)
	{
		return SW_ERR;
	}
	
	have_attr = 0;
	pstNode2 = pstNode1->firstchild;
	while(pstNode2 != NULL)
	{
		if (pstNode2->node_type == SW_NODE_ATTRIB)
		{
			have_attr = 1;
			result = xmlmem_write(fpt,type," %s=\"%s\"",pstNode2->name, pstNode2->value);
			if (result != SW_OK)
			{
				return SW_ERR;
			}
		}
		else
		{
			/*���ӽڵ�*/
			break;
		}
		pstNode2 = pstNode2->next;
	}

	if (pstNode2 != NULL)
	{
		/*���ӽڵ�*/
		result = xmlmem_write(fpt, type, ">\n");
		if (result != SW_OK)
		{
			return SW_ERR;
		}
		
		while(pstNode2 != NULL)
		{
			xml_savenode(pstNode2,iLevel + 1,fpt,type);
			pstNode2 = pstNode2->next;
		}

		if (pstNode1->value != NULL)
		{
			if (iLevel > 0)
			{
				for(i = 0;i < iLevel;i++)
				{
					result = xmlmem_write(fpt, type, "\t");
					if (result != SW_OK)
					{
						return SW_ERR;
					}
				}
			}
			
			if (xmlbuf_cpy(pstNode1->value) < 0)
			{
				pub_log_error("[%s][%d] Xmlbuf cpy error!", __FILE__, __LINE__);
				return SW_ERR;
			}
			xmlmem_write(fpt, type, "%s\n", g_xmlvalue);
			xmlbuf_free();
		}

		if (iLevel > 0)
		{
			for(i = 0;i < iLevel;i++)
			{
				xmlmem_write(fpt, type, "\t");
			}
		}

		xmlmem_write(fpt, type, "</%s>\n", pstNode1->name);
	}
	else
	{
		/*���ӽڵ�*/
		if (pstNode1->value != NULL && pstNode1->value[0] != '\0')
		{
			if (xmlbuf_cpy(pstNode1->value) < 0)
			{
				pub_log_error("[%s][%d] Xmlbuf cpy error!", __FILE__, __LINE__);
				return SW_ERR;
			}
			xmlmem_write(fpt, type, ">%s</%s>\n", g_xmlvalue, pstNode1->name);
			xmlbuf_free();
		}
		else if (have_attr == 0)
		{
			/*** û������ ***/
			xmlmem_write(fpt, type, "></%s>\n", pstNode1->name);
		}
		else
		{
			/*** ������ ***/
			xmlmem_write(fpt, type, "/>\n");
		}
	}

	return SW_OK;
}



static sw_xmltree_t * xml_crtree(sw_xmlmem_t *fp,int type)
{	
	int idx;
	int flg;
	int pos;
	int nextpos;
	char	*line = NULL;
	char	*value = NULL;
	char	name[ITEM_NAME_LEN];
	char readbuf[XML_READ_BUF_MAX];
	char attrname[ITEM_NAME_LEN];
	char attrvalue[ITEM_VALUE_LEN];
	sw_buf_t	*pbuf = NULL, *vbuf = NULL;
	sw_xmlnode_t *node;
	sw_xmlnode_t *root;
	sw_xmlnode_t *cur;
	sw_xmltree_t *xmltree;

	memset(name,0x00,sizeof(name));
	memset(readbuf,0x00,sizeof(readbuf));
	memset(attrname,0x00,sizeof(attrname));
	memset(attrvalue,0x00,sizeof(attrvalue));
	
	/*��������*/
	root = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t)) ;
	if (root == NULL)
	{
		pub_log_error("[%s][%d],allocted mem error,[%d]",__FILE__,__LINE__,errno);
		return NULL;
	}
	xml_initnode(root);
	root->name = (char *)malloc(5);
	if (root->name == NULL)
	{
		free(root);
		pub_log_error("[%s][%d],allocted mem error,[%d]",__FILE__,__LINE__,errno);
		return NULL;
	}
	strcpy(root->name,"ROOT");
	root->value = (char *)malloc(strlen("ROOT") + 1);
	if (root->value == NULL)
	{
		free(root->name);
		free(root);
		pub_log_error("[%s][%d],allocted mem error,[%d]",__FILE__,__LINE__,errno);
		return NULL;
	}
	strcpy(root->value,"ROOT");
	root->node_type = SW_NODE_ROOT;/*Ϊģ��oracle 9i��������*/

	/*�����֧���*/
	cur = root;
	xmltree = (sw_xmltree_t *)malloc(sizeof(sw_xmltree_t));
	if (xmltree == NULL)
	{
		pub_xml_delnode(root);
		pub_log_error("[%s][%d],allocted mem error,[%d]",__FILE__,__LINE__,errno);
		goto ERR_EXIT;
	}
	xmltree->root = root;
	xmltree->current = root;

	nextpos = 0;
	flg = SW_S_CHILD;
	memset(readbuf,0x00,sizeof(readbuf));
	
	pbuf = buf_new();
	if (pbuf == NULL)
	{
		pub_log_error("[%s][%d] Alloc buffer error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		pub_xml_delnode(root);
		return NULL;
	}

	vbuf = buf_new();
	if (vbuf == NULL)
	{
		pub_log_error("[%s][%d] Alloc buffer error! errno=[%d]:[%s]",
			__FILE__, __LINE__, errno, strerror(errno));
		pub_xml_delnode(root);
		buf_release(pbuf);
		return NULL;
	}

	while(1)
  	{
		buf_refresh(pbuf);
		buf_refresh(vbuf);
	  	xml_readline(fp, pbuf, readbuf, &nextpos, type);
		line = pbuf->data;
		if (line[0] == '\0')
		{
			break;
		}
		
		pos = xmlline_getvalue(line, vbuf);
		value = vbuf->data;
		idx = 0;
		while((value[idx] == ' ')&&(value[idx] != '\0'))
		{
			idx++;
		}

		if (value[idx] == '\0')
		{
			value[0] = '\0';
		}

		pos = xmlline_getname(line,name,pos);
		if (value[0] != '\0')
		{/**��value**/
			if (flg == SW_S_BROTHER)
			{	/*�ڴ��ֵ�*/
				cur = cur->parent;
				if (cur == NULL)
				{
					pub_log_error("[%s][%d],xmltreeaddr=[%d]",__FILE__,__LINE__,xmltree);
					goto ERR_EXIT;
				}			
			}			
			if (xml_dealvalue(value) != NULL)
			{
				if (cur->value != NULL)
				{
					pub_log_error("[%s][%d] Format error!!!!!", __FILE__, __LINE__);
					goto ERR_EXIT;
				}
				cur->value = (char *)malloc(strlen(value) + 1);
				CHECKPTR(cur->value);
				strcpy(cur->value,value);
			}
			flg = SW_S_BROTHER;
			continue;
	 	}
	 	else
	 	{	/*û��value��*/
			if (name[0] != '/')
			{	/*�ǿ�ʼ��*/
				node = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
				CHECKPTR(node);
				xml_initnode(node);
				node->node_type = SW_NODE_ELEMENT;/*Ϊģ��oracle9i��������*/
				node->name = (char *)malloc(strlen(name) + 1);
				CHECKPTR(node->name);
				strcpy(node->name,name);
				if (flg == SW_S_BROTHER)
				{
					if (cur->next != NULL)
					{
						pub_log_error("[%s][%d],xmltreeaddr=[%d]",__FILE__,__LINE__,xmltree);
						goto ERR_EXIT;
					}
					cur->next = node;
					node->parent = cur->parent;
				}
				else
				{
					if (cur->firstchild != NULL)
					{
						pub_log_error("[%s][%d],firstchild == NULL",__FILE__,__LINE__);
						goto ERR_EXIT;
					}
					cur->firstchild = node;
					node->parent = cur;
				}
#ifdef XML_PACK_NULL	
				if (strncmp(line + pos, "/>", 2) == 0)
				{
					cur = node;
					cur->next = NULL;
					flg = SW_S_BROTHER;
					continue;
				}
#endif
				cur = node;
				pos = xmlline_getattr(line,attrname,attrvalue,pos);
				if (pos >= 0)
				{	/*������*/
					node = (sw_xmlnode_t *)malloc(sizeof( sw_xmlnode_t));
					CHECKPTR(node);
					xml_initnode(node);
					node->node_type = SW_NODE_ATTRIB;/*Ϊģ��oracle 9i��������*/
					node->name = (char *)malloc(strlen(attrname) + 1);
					CHECKPTR(node->name);
					strcpy(node->name,attrname);
					node->value=(char *)malloc(strlen(attrvalue) + 1);
					CHECKPTR(node->value);
					strcpy(node->value,attrvalue);
					if (cur->firstchild != NULL)
					{
						pub_log_error("[%s][%d],firstchild != NULL",__FILE__,__LINE__);
						goto ERR_EXIT;
					}

					cur->firstchild = node;
					node->parent = cur;
					cur = node;

					while(1)
					{
						pos = xmlline_getattr(line,attrname,attrvalue,pos);
						if (pos < 0)
						{
							break;
						}
						node = (sw_xmlnode_t *)malloc(sizeof( sw_xmlnode_t));
						CHECKPTR(node);
						xml_initnode(node);
						node->node_type = SW_NODE_ATTRIB;/*Ϊģ��oracle 9i��������*/
						node->name = (char *)malloc(strlen(attrname) + 1);
						CHECKPTR(node->name);
						strcpy(node->name,attrname);
						node->value=(char *)malloc(strlen(attrvalue) + 1);
						CHECKPTR(node->value);
						strcpy(node->value,attrvalue);
						if (cur->next != NULL)
						{
							pub_log_error("[%s][%d],cur->next!=NULL",__FILE__,__LINE__);
							goto ERR_EXIT;
						}
						cur->next = node;
						node->parent = cur->parent;
						cur = node;
					}
					if (pos == SW_S_ALL_END)
					{	/*�н�����־*/
						cur = cur->parent;
						flg = SW_S_BROTHER;
						continue;
					}
					else
					{	/*û�н�����־*/
						flg = SW_S_BROTHER;
						continue;
					}
				}
				else
				{	/*û������*/
					flg = SW_S_CHILD;
					continue;
				}
			}
			else
			{	
				/*�ǽ�����*/
				if (cur == NULL)
				{
					pub_log_error("[%s][%d],cur==NULL",__FILE__,__LINE__);
					goto ERR_EXIT;
				}
				if (flg == SW_S_BROTHER)
				{
					cur = cur->parent;
				}
							

				while(strcmp(cur->name,name + 1) != 0)
				{
					cur = cur->parent;
					if (cur == NULL)
					{
						pub_log_error("[%s][%d],xmltreeaddr=[%d]",__FILE__,__LINE__,xmltree);
						goto ERR_EXIT;
					}
				}
				flg = SW_S_BROTHER;
				continue;
			}/*end ��ʼ��*/
		}/*end if else û��value��*/
	}/*end while*/
	buf_release(pbuf);
	buf_release(vbuf);

	return xmltree;
ERR_EXIT:
	pub_log_error("[%s][%d] Error!!!!", __FILE__, __LINE__);
	if (xmltree != NULL)
	{
		pub_xml_deltree(xmltree);
		xmltree=NULL;
	}
	buf_release(pbuf);
	buf_release(vbuf);
			
	return NULL;

}


/******************************************************************************
  *** ��������: pub_xml_crtree
  *** ��������: �������ļ��ж����һ�������Լ���ǰ�Ľ�㣬�����в���һ�����
  ***		 flg��־�½���뵱ǰ���Ĺ�ϵ��SW_S_CHILDΪ����,SW_S_BROTHERΪ�ֵ�
  ***		 ����ֵΪ�½���ַ��������������/>��ʽ�Ļ����У��򷵻ػ��ݵ��Ľ��)
  ***		 flg�ڷ���ʱ�����ó���һ���뵱ǰ�еĹ�ϵ
  *** ��������: Shi Shenghui
  *** ��������: 2008-1-29 16:19
  *** ���ú���: ��	
  *** ���ʵı�: ��
  *** �޸ĵı�: ��
  *** �������: 
  ***	 ����1: psFileName		 �ļ�·��ָ��
  *** �������: ��
  *** �� �� ֵ: 0:�ɹ�����0:ʧ��
  *** ע������: 
  ******************************************************************************/
sw_xmltree_t * pub_xml_crtree(char *file_name)
{
	int ret;
	sw_xmlmem_t fp;
	sw_xmltree_t *xmltree;
	
	if(file_name == NULL || file_name[0] == '\0')
	{
		pub_log_error("[%s][%d],file check error(not found),[%s]",__FILE__,__LINE__,file_name);
		return NULL;
	}

	ret = xmlmem_init(&fp, file_name,XML_FILE_TYPE);
	if (ret <0 )
	{
		pub_log_error("[%s][%d],xmlmem_init error,[%s] [%d]",__FILE__,__LINE__,file_name,errno);
		return NULL;
	}

	xmltree = xml_crtree(&fp, XML_FILE_TYPE);

	xmlmem_clear(&fp, XML_FILE_TYPE);

	return xmltree;
}
 sw_xmltree_t * pub_xml_crtree_ext(char *pack,int iLen)
 {
	 int ret;
	 sw_xmlmem_t fp;
	 sw_xmltree_t *xmltree;


	ret = xmlmem_init(&fp, pack, XML_MEM_TYPE);
	if (ret <0 )
	{
		pub_log_error("[%s][%d],xmlmem_init error [%d]",__FILE__,__LINE__,errno);
		return NULL;
	}
	if(iLen > 0)
	{	 
		fp.length = iLen;
	}
	xmltree = xml_crtree(&fp, XML_MEM_TYPE);

	xmlmem_clear(&fp, XML_MEM_TYPE);

	return xmltree;
}



/******************************************************************************
 *** ��������: pub_xml_locnode
 *** ��������: ��λĳ�����,���"item.name.col"�������".item.name.col",
 ***		����·���ı䵱ǰ·�������·�����ı䵱ǰ·��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 16:2
 *** ���ú���: �� 
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: pstXml		���ṹ��ָ��
 *** 	����1: psLoc		��ѯ�Ľڵ�λ��
 *** �������: 
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
sw_xmlnode_t *pub_xml_locnode(sw_xmltree_t *pstXml,char *psLoc)
{
	int	len = 0;
	int	iPos1 = 0;
	int	iPos2 = 0;
	sw_xmlnode_t *node;

	if(pstXml == NULL||psLoc == NULL)
	{
		return NULL;
	}

	if (strcmp(psLoc,".") == 0)
	{
		return pstXml->root;
	}

	if (psLoc[0] == '.')
	{	
		/*����·��*/
		node = pstXml->root;
		iPos1 = 1;
		iPos2 = 1;
	}
	else
	{	
		/*���·��*/
		node = pstXml->current;
		iPos1 = 0;
		iPos2 = 0;
	}

	if(node == NULL)
	{
		pub_log_error("[%s][%d],xmltree,node error,[0x%x][%s]",__FILE__,__LINE__,pstXml,psLoc);
		return NULL;
	}

	if (node->firstchild == NULL)
	{
		/*pub_log_error("[%s][%d],xmltree,child node error,[0x%x][%s]",__FILE__,__LINE__,pstXml,psLoc);*/
		return NULL;
	}

	node = node->firstchild;
	while(iPos2 >= 0)
	{
		while((psLoc[iPos2] != '.')&&(psLoc[iPos2] !=  '\0'))
		{
			iPos2++;
		}

		while(node)
		{
			len = strlen(node->name);
			if (len == (iPos2-iPos1))
			{
				if (memcmp(node->name,psLoc + iPos1,iPos2 - iPos1) == 0)
				{
					break;
				}
			}
			node = node->next;
		}

		if (node == NULL)
		{
			/*pub_log_error("[%s][%d],path error,[0x%x][%s]",__FILE__,__LINE__,pstXml,psLoc);*/
			return NULL;
		}

		if (psLoc[iPos2] == '\0')
		{
			iPos2 = -1;
		}
		else
		{
			iPos1 = iPos2 + 1;
			iPos2 = iPos1;
			node = node->firstchild;
			if (node == NULL)
			{
				/*pub_log_error("[%s][%d],path error,[0x%x][%s]",__FILE__,__LINE__,pstXml,psLoc);*/
				return NULL;
			}
		}
	}
	if (psLoc[0] == '.')
	{
		pstXml->current = node;
	}

	return node;
}



/******************************************************************************
 *** function  : pub_xml_loc_tag
 *** author    : zhang hailu
 *** create    : 2013-5-20 9:14
 *** call lists:  
			pub_xml_locnode
 *** inputs    : 
 ***     arg1  :  xmltree	xml tree
 ***	 arg2  :  targ		the node path  like "NAME.TARGET"
 ***	 arg3  :  value		the value of targ
 *** outputs   :  
 ***     arg1  : 
 *** return    :  sw_xmlnode_t:success  NULL:fail
 *** notice    : 
 ***   author  : 
 ***   date    : 
 ***   content : 
 ******************************************************************************/
sw_xmlnode_t *pub_xml_locnode_value(sw_xmltree_t *xmltree,char *targ,char *value)
{
	sw_xmlnode_t *poBackCurrent;
	sw_xmlnode_t *pnode1;
	sw_xmlnode_t *pnode2;

	pnode1=NULL;
	pnode2=NULL;
	poBackCurrent=NULL;

	if(xmltree==NULL||value==NULL||strlen(value)==0
			||targ==NULL||strlen(targ)==0)
	{
		return(NULL);
	}
	poBackCurrent=xmltree->current;
	pnode1=xmltree->current;
	for (;pnode1!=NULL;pnode1=pnode1->next)
	{
		xmltree->current=pnode1;		
		pnode2=pub_xml_locnode(xmltree,targ);
		if(pnode2==NULL || pnode2->value[0] == '\0')
		{
			continue;
		}
		else if(strcmp(pnode2->value,value)==0)
		{
			break;
		}
	}
	xmltree->current=poBackCurrent;
		
	return((pnode2&&pnode1)?pnode1:pnode2);
	
}

/****����һ������****/
sw_xmlnode_t *pub_xml_addnode(sw_xmlnode_t *node,char *name,char *value,int flg)
{
	sw_xmlnode_t *pnode1,*pnode_pre;
	sw_xmlnode_t *pnew;
	
	pnode1=node->firstchild;
	pnode_pre=pnode1;
	
	while(pnode1!=NULL)
	{
		if(flg != SW_NODE_ATTRIB
			|| pnode1->node_type ==SW_NODE_ATTRIB)
		{
			pnode_pre=pnode1;
		}
		pnode1=pnode1->next;
	}
	pnew=(sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
	memset(pnew,0,sizeof(sw_xmlnode_t));
	pnew->name=(char *)malloc(strlen(name)+1);
	strcpy(pnew->name,name);
	pnew->value=(char *)malloc(strlen(value)+1);
	strcpy(pnew->value,value);

	pnew->node_type = flg;
	if(pnode_pre==NULL)
	{
		node->firstchild=pnew;
		pnew->parent=node;
	}
	else if(pnode_pre->node_type!=SW_NODE_ATTRIB
		&& flg == SW_NODE_ATTRIB)
	{
		pnew->next=node->firstchild;
		node->firstchild=pnew;
		pnew->parent=node;
	}
	else
	{
		pnew->next=pnode_pre->next;
		pnode_pre->next=pnew;
		pnew->parent=node;
	}
	return(pnew);
}


/******************************************************************************
 *** ��������: pub_xml_crnode
 *** ��������: ����ָ����ǩ��֧�ڵ�
 *** ��������: zhang hailu
 *** ��������: 2011-08-01
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: pstXml		��Ҫ������ǩ����
 *** 	����2: name		�����ڵ�ı�ǩ����.NAME.FLAG ������FLAG֧�ڵ㣬
 ***					��NAME������ͬʱ����NAME֧�ڵ㣩
 ***    ����4: flg		�Ƿ�Ϊ���Ա�ǩ
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
sw_xmlnode_t * pub_xml_crnode(sw_xmltree_t *pstXml,char *name,int flg)
{
	int	iPos1 = 0;
	int	iPos2 = 0;
	sw_xmlnode_t *node;
	sw_xmlnode_t *pstBakNode;
	sw_xmlnode_t *pstNode;

	if (strcmp(name,".") == 0)
	{
		return pstXml->root;
	}
	if (name[0] == '.')
	{	
		/*����·��*/
		iPos1 = 1;
		iPos2 = 1;
		node = pstXml->root;
	}
	else
	{	
		/*���·��*/
		node = pstXml->current;
		iPos1 = 0;
		iPos2 = 0;
	}

	while(iPos2 >= 0)
	{
		while((name[iPos2] != '.')&&(name[iPos2] != '\0'))
		{
			iPos2++;
		}

		if (node->firstchild == NULL)
		{
			pstBakNode = NULL;
			pstBakNode = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
			if (pstBakNode == NULL)
			{
				return NULL;
			}
			xml_initnode(pstBakNode);
			if ((flg == 1)&&(name[iPos2] == '\0'))
			{
				pstBakNode->node_type  = SW_NODE_ATTRIB;
			}
			else
			{
				pstBakNode->node_type  = SW_NODE_ELEMENT;
			}
			pstBakNode->parent = node;
			node->firstchild = pstBakNode;
			node = pstBakNode;

			node->name = (char *)malloc(iPos2 - iPos1 + 1);
			if (node->name == NULL)
			{
				return NULL;
			}
			memset(node->name,0x00,iPos2 - iPos1 + 1);
			strncpy(node->name,name + iPos1,iPos2 - iPos1);
			if (name[iPos2] == '\0')
			{
				break;
			}
			else
			{
				iPos1 = iPos2 +1;
				iPos2 = iPos1;
			}
			continue;
		}
		pstBakNode = node;
		node = node->firstchild;
		while(node != NULL)
		{
			if (strlen(node->name) == (size_t)(iPos2 - iPos1))
			{
				if (memcmp(node->name,name + iPos1,iPos2 - iPos1) == 0)
				{
					if (name[iPos2] == '\0')
					{
						pstBakNode = NULL;
						pstBakNode = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
						if (pstBakNode == NULL)
						{
							return NULL;
						}
						xml_initnode(pstBakNode);
						if (flg == 1)
						{
							pstBakNode->node_type  = SW_NODE_ATTRIB;
						}
						else
						{
							pstBakNode->node_type  = SW_NODE_ELEMENT;
						}
						pstBakNode->parent = node->parent;
						node->next = pstBakNode;
						node = pstBakNode;
						node->name = (char *)malloc(iPos2 - iPos1 + 1);
						memset(node->name,0x00,iPos2 - iPos1 + 1);
						strncpy(node->name,name + iPos1,iPos2 - iPos1);
					}
					break;
				}
			}
			node = node->next;
		}

		if (node == NULL)
		{
			pstNode = NULL;
			pstNode = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
			if (pstNode == NULL)
			{
				return NULL;
			}
			xml_initnode(pstNode);
			if ((flg == 1)&&(name[iPos2] == '\0'))
			{
				pstNode->node_type  = SW_NODE_ATTRIB;
			}
			else
			{
				pstNode->node_type  = SW_NODE_ELEMENT;
			}
			node = pstNode;
			node->parent = pstBakNode;
			node->next       = NULL;
			node->firstchild = NULL;
			pstBakNode = pstBakNode->firstchild;
			while(pstBakNode->next != NULL)
			{
				pstBakNode = pstBakNode->next;
			}
			pstBakNode->next = node;

			node->name = (char *)malloc(iPos2 - iPos1 + 1);
			memset(node->name,0x00,iPos2 - iPos1 + 1);
			strncpy(node->name,name + iPos1,iPos2 - iPos1);
		}

		if (name[iPos2] == '\0')
		{
			break;
		}
		else
		{
			iPos1 = iPos2 + 1;
			iPos2 = iPos1;
		}
	}
	if (name[0] == '.')
	{
		pstXml->current = node;
	}

	return node;
}

/******************************************************************************
 *** ��������: pub_xml_delnode
 *** ��������: �����ǰ�ڵ�
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 16:43
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: node		��ǰ�ڵ�ָ��
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/

void pub_xml_delnode(sw_xmlnode_t *node)
{
	if (node == NULL)
	{
		return;
	}

	if (node->next != NULL)
	{
		pub_xml_delnode(node->next);
		node->next = NULL;
	}

	if (node->firstchild != NULL)
	{
		pub_xml_delnode(node->firstchild);
		node->firstchild = NULL;
	}

	if (node->name != NULL)
	{
		free(node->name);
		node->name = NULL;
	}

	if (node->value != NULL)
	{
		free( node->value);
		node->value = NULL;
	}
	free( node);
	node = NULL;
}

/******************************************************************************
 *** ��������: pub_xml_unpack
 *** ��������: �齨xml��
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 18:32
 *** ���ú���: 
 *** 	����1: pub_xml_crtree
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: psPack		�ļ�·��ָ��
 *** �������: ��
 *** �� �� ֵ: ���ṹָ��
 *** ע������: 
 ******************************************************************************/
sw_xmltree_t * pub_xml_unpack(char *psPack)
{
	sw_xmlnode_t *root;
	sw_xmltree_t *pstXml;
	
	/*�����ʼ��һ����XML��*/
	if (psPack == NULL)
	{
		root = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t)) ;
		xml_initnode(root);
		root->name = (char *)malloc(5);
		strcpy(root->name,"ROOT");
		root->value = (char *)malloc(7);
		strcpy(root->value,"SEAGLE");
		root->node_type = SW_NODE_ROOT;/*Ϊģ��oracle 9i��������*/
		pstXml = (sw_xmltree_t *)malloc(sizeof(sw_xmltree_t));
		pstXml->root = root;
		pstXml->current = root;
		pub_log_info("[%s][%d],create root xml success,[%x]",__FILE__,__LINE__,pstXml);
		return(pstXml);
	}
	return (pub_xml_crtree(psPack));
}

sw_xmltree_t * pub_xml_unpack_ext(char *pack,int iLen)
{
	sw_xmlnode_t *proot;
	sw_xmltree_t *pxml;
	

	/****�����ʼ��һ����XML��****/
	if(pack==NULL){
		proot=(sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
		xml_initnode(proot);
		proot->name=(char *)malloc(5);
		strcpy(proot->name,"ROOT");
		proot->value=(char *)malloc(7);
		strcpy(proot->value,"SEAGLE");
		proot->node_type=SW_NODE_ROOT;/*Ϊģ��oracle 9i��������*/
		pxml=(sw_xmltree_t *)malloc(sizeof(sw_xmltree_t));
		pxml->root=proot;
		pxml->current=proot;
		return(pxml);
	}

	return pub_xml_crtree_ext(pack,iLen);

}

/******************************************************************************
 *** ��������: pub_xml_pack
 *** ��������: ��xml�����ļ�
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 18:35
 *** ���ú���: 
 *** 	����1: pub_xml_pack
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: pstXml			xml��ָ��
 *** 	����2: psPack			�ļ�·��ָ��
 *** �������: ��
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 ******************************************************************************/
sw_int_t pub_xml_pack(sw_xmltree_t *pstXml,char *psPack)
{
	int iRc;
	sw_xmlnode_t *node1;
	sw_xmlmem_t fp;	

	if ((psPack == NULL)||(psPack[0] == '\0'))
	{
		pub_log_error("[%s][%d],pub_xml_pack input error,[%x]",__FILE__,__LINE__,psPack);
		return -1;
	}

	if (pstXml == NULL)
	{
		pub_log_error("[%s][%d],pub_xml_pack input error,[%x]",__FILE__,__LINE__,pstXml);
		return -1;
	}

	iRc=xmlmem_init_ext(&fp,psPack,XML_FILE_TYPE);
	if(iRc< 0)
	{
		pub_log_error("[%s][%d]Can open file[%s]", __FILE__, __LINE__,psPack);
		return(-1);
	}
	if (pstXml->root == NULL)
	{
		xmlmem_clear(&fp,XML_FILE_TYPE);
		pub_log_error("[%s][%d],pub_xml_pack root error,[%x]",__FILE__,__LINE__,pstXml);
		return -1;
	}
	node1 = pstXml->root->firstchild;
	
	while(node1 != NULL)
	{
		iRc = xml_savenode(node1,0,&fp,XML_FILE_TYPE);
		if (iRc < 0)
		{
			xmlmem_clear(&fp,XML_FILE_TYPE);	
			pub_log_error("[%s][%d],save node error,iRc=[%d],[%x]",__FILE__,__LINE__,iRc,pstXml);
			return iRc;
		}
		node1 = node1->next;
	}
	xmlmem_clear(&fp,XML_FILE_TYPE);
	return 0;
}




sw_int_t pub_xml_pack_ext(sw_xmltree_t *pxml,char *pack)
{
	sw_xmlmem_t fp;
	sw_xmlnode_t *pnode1 = NULL;
	int iRc;
	iRc=xmlmem_init_ext(&fp,pack,XML_MEM_TYPE);
	if(iRc< 0)
	{
		pub_log_error("Can not open sw_xmlmem_t\n");
		return(-1);
	}
	if(pxml==NULL){
		pub_log_error("%s,%d,XML��������\n",__FILE__,__LINE__);
		xmlmem_clear(&fp,XML_MEM_TYPE);
		return(-1);
	}
	if(pxml->root==NULL){
		pub_log_error("%s,%d,XML��������\n",__FILE__,__LINE__);
		xmlmem_clear(&fp,XML_MEM_TYPE);
		return(-1);
	}
	
	pnode1=pxml->root->firstchild;
	while(pnode1!=NULL)
	{
		pub_log_debug("[%s][%d] node->name=[%s]", __FILE__, __LINE__, pnode1->name);
		iRc=xml_savenode(pnode1,0,&fp,XML_MEM_TYPE);	
		if(iRc<0)
		{
			xmlmem_clear(&fp,XML_MEM_TYPE);			
			return(iRc);
		}
		pnode1=pnode1->next;
	}
	xmlmem_clear(&fp,XML_MEM_TYPE);
	return(0);
}

sw_int_t pub_xml_pack_node(sw_xmltree_t *pxml, sw_xmlnode_t *node, char *pack)
{
	int	ret = 0;
	sw_xmlmem_t	fp;
	sw_xmlnode_t	*pnode1 = NULL;

	if (pxml == NULL)
	{
		pub_log_error("[%s][%d] XML tree not exist!", __FILE__, __LINE__);
		return -1;
	}

	if (node == NULL)
	{
		pub_log_error("[%s][%d] Pack node is null!", __FILE__, __LINE__);
		return -1;
	}

	ret = xmlmem_init_ext(&fp, pack, XML_MEM_TYPE);
	if (ret < 0)
	{
		pub_log_error("[%s][%d] XML meminit error!", __FILE__, __LINE__);
		return -1;
	}

	pnode1 = node;
	ret = xml_savenode(pnode1, 0, &fp, XML_MEM_TYPE);
	if (ret < 0)
	{
		xmlmem_clear(&fp, XML_MEM_TYPE);
		return ret;
	}

	if (fp.length > 0 && (pack[fp.length - 1] == 0xa || pack[fp.length - 1] == 0xd))
	{
		pack[fp.length - 1] = '\0';
	}

	xmlmem_clear(&fp, XML_MEM_TYPE);

	return 0;
}

/* ���ؼ����XML���ĵĳ��� */
sw_int_t pub_xml_calc_len(sw_xmlnode_t *pnode_cur, int level)
{
	int	i = 0, length = 0, length2 = 0;
	int	have_attr = 0;
	sw_xmlnode_t *pnode1,*pnode2;
	
	if(pnode_cur==NULL){
		pub_log_error("%s,%d,XML��������\n",__FILE__,__LINE__);
		return(-1);
	}
	if(pnode_cur->node_type==SW_NODE_ATTRIB){
		/****���������������ڵ�****/
		pub_log_error("%s,%d,XML��������\n",__FILE__,__LINE__);
		return(-1);
	}
	
	if (level == 0 && pnode_cur->firstchild == NULL)
	{
		return 0;
	}

	pnode1=pnode_cur;
	if(level>0){
		for(i=0;i<level;i++){
			/*** TAB�� ***/
			length += 1;
		}
	}
	
	/*** <NAME ***/
	have_attr = 0;
	length += 1 + strlen(pnode1->name);
	pnode2=pnode1->firstchild;
	while(pnode2!=NULL)
	{
		if(pnode2->node_type==SW_NODE_ATTRIB)
		{
			/*** NAME="VALUE" ***/
			have_attr = 1;
			length += strlen(pnode2->name) + 4 + strlen(pnode2->value);
		}
		else
		{
			/****���ӽڵ�****/
			break;
		}
		pnode2=pnode2->next;
	}
	if(pnode2!=NULL)
	{
		/****���ӽڵ�****/
		/*** >\n ***/
		length += 2;
		while(pnode2!=NULL)
		{
			length2 = pub_xml_calc_len(pnode2,level+1);
			if(length2 < 0)
			{
				return -1;
			}
			length += length2;
			pnode2=pnode2->next;
		}
		if(pnode1->value!=NULL){
			if(level>0){
				for(i=0;i<level;i++){
					/*** TAB�� ***/
					length += 1;
				}
			}
			/****���ǹؼ��ֵ�ת��****/
			/****
			sprintf(fp->start+fp->length,"%s\n",pnode1->value);
			****/
			/*** VALUE\n ***/
			length += xml_strlen(pnode1->value) + 1;
		}
		if(level>0){
			for(i=0;i<level;i++){
				/*** TAB�� ***/
				length += 1;
			}
		}
		/*** </NAME>\n ***/
		length += 4 + strlen(pnode1->name);
	}else{
		/****���ӽڵ�****/
		if(pnode1->value!=NULL){
			/*** >\n ***/
	#if 0
			length += 2;
			if(level>0){
				for(i=0;i<level;i++){
					/*** TAB�� ***/
					length += 1;
				}
			}
			/****���ǹؼ��ֵ�ת��****/
			/****
			sprintf(fp->start+fp->length,"%s\n",pnode1->value);
			****/
			length += strlen(pnode1->value) + 1;
			if(level>0){
				for(i=0;i<level;i++){
					/*** TAB�� ***/
					length += 1;
				}
			}
			length += 4 + strlen(pnode1->name);
	#endif
			length += 5 + strlen(pnode1->name) + xml_strlen(pnode1->value);
		}
		else if (have_attr == 0)
		{
			length += 5 + strlen(pnode1->name);
		}
		else
		{
			/*** />\n ***/
			length += 3;
		}
	}
	
	return length;	
}
/******************************************************************************
 *** ��������: pub_xml_prt
 *** ��������: ��ӡ��ǰxml���ڵ㼰����ȫ���ڵ�
 *** ��������: Shi Shenghui
 *** ��������: 2008-1-29 16:0
 *** ���ú���: ��
 *** ���ʵı�: ��
 *** �޸ĵı�: ��
 *** �������: 
 *** 	����1: node		��ǰ�ڵ�ָ��
 *** �������: ��
 *** �� �� ֵ: void
 *** ע������: 		
 ******************************************************************************/
sw_int_t pub_xml_prt(sw_xmlnode_t *node)
{
	/*
	printf("name:%s,parent:%x,next:%x,child:%x\n",node->name,node->parent,node->next,node->firstchild);
	getchar();
	*/

	if (node == NULL)
	{
		return SW_OK;
	}
	printf("%s,%s\n",node->name,node->value);
	pub_xml_prt(node->firstchild);
	pub_xml_prt(node->next);
	
	return SW_OK;
}


sw_int_t pub_xml_getpath(sw_xmlnode_t *pnode,char *path)
{
	int name_len;
	int length;
	char name[32];
	length=0;
	while(pnode!=NULL)
	{
		if(pnode->node_type==SW_NODE_ROOT)
		{
			break;
		}
		name_len=sprintf(name,".%s",pnode->name);
		pub_mem_memmove(path+name_len,path,length);
		memcpy(path,name,name_len);
		length+=name_len;
		pnode=pnode->parent;
	}
	return(SW_OK);
}



/******************************************************************************
 *** ��������: pub_xml_get_nodevalue
 *** ��������: �õ���ǰ��ǩ���ӱ�ǩ�����Ի��ӱ�ǩ��ֵ
 *** ��������: liteng
 *** ��������: 2011-11-29 18:45
 *** ���ú���: 
 *** 	����1: pub_xml_locnode
 *** ���ʵı�: 
 *** �޸ĵı�: 
 *** �������: 	
	 *** 	����1: pstXml 		xml���ṹָ��
 *** 	����2: psMark 		��ǰ��ǩ���ӱ�ǩ������
 ***    ����3: psNodeName   	��ǰ��ǩ���ӱ�ǩ�����Ի��ӱ�ǩ������
 ***    ����4: psMark   	��ǰ��ǩ���ӱ�ǩ������
 *** �������: 
 *** 	����1: sValue		��ǰ��ǩ���ӱ�ǩ�����Ի��ӱ�ǩ��ֵ
 *** �� �� ֵ: 0:�ɹ�����0:ʧ��
 *** ע������: 
 *** ʹ��˵����
 ***<global>
 ***	<infoserver name="swcomm" maxnum = "3" minmun ="10" >
 ***		<out>mytest hello</out>
 ***	</infoserver>
 ***</global>
 ***��ǰ��ǩ<global>���ӱ�ǩ<infoserver>���ӱ�ǩ������name���ӱ�ǩ���ӱ�ǩ<out>
 ***���������psMark="infoserver",psNodeName="name",�����sValue="swcomm"
 ***���������psMark="infoserver",psNodeName="out",�����sValue="mytest hello"
 ******************************************************************************/
sw_int_t pub_xml_get_nodevalue(sw_xmltree_t *pstXml,char *psMark,char *psNodeName,char *psValue)
{
	char	*psStr = NULL;
	sw_xmlnode_t	*pstNode1 = NULL;
	sw_xmlnode_t	*pstNode2 = NULL;
	sw_xmlnode_t	*pstBackCurrent = NULL;

	if ((pstXml == NULL)||(psMark == NULL)||(psMark[0] == '\0')||
		(psNodeName == NULL)||psNodeName[0] == '\0')
	{
		return -1;
	}
	pstBackCurrent = pstXml->current;

	psStr = strrchr(psMark,46);
	if (psStr == NULL)
	{
		pstNode2 = pstXml->current->firstchild;
	}
	else
	{
		pstNode2 = pub_xml_locnode(pstXml,psMark);
		memcpy(psMark,psStr + 1,strlen(psStr) + 1);
		psStr = NULL;
	}

	while(pstNode2 != NULL)
	{
		if (memcmp(pstNode2->name,psMark,strlen(psMark)) == 0)
		{
			break;
		}
		pstNode2 = pstNode2->next;
	}

	if (pstNode2 == NULL)
	{
		pstXml->current = pstBackCurrent;
		return -1;
	}

	pstNode1 = pstNode2;
	while(pstNode1 != NULL)
	{
		pstXml->current = pstNode1;
		pstNode2 = pub_xml_locnode(pstXml,psNodeName);
		if (pstNode2 != NULL)
		{
			/*printf("[%s][%d],pstNode2->name=[%s],pstNode2->vlaue=[%s],psNodeName=[%s]\n",__FILE__,__LINE__,pstNode2->name,pstNode2->value,psNodeName);*/
			memcpy(psValue,pstNode2->value,strlen(pstNode2->value));
			break;
		}

		pstNode1 = pstNode1->next;
	}

	pstXml->current = pstBackCurrent;
	return 0;
}

/******************************************************************************
 **��������: pub_xml_set_value
 **��    ��: ���ýڵ�ֵ
 **�������:
 **     node: XML�ڵ�
 **     value: �ڵ�ֵ
 **�������:
 **��    ��: 0:�ɹ� !0:ʧ��
 **ʵ������: 
 **ע������: 
 **��    ��: # Qifeng.zou # 2013.06.12 #
 ******************************************************************************/
sw_int_t pub_xml_set_value(sw_xmlnode_t *node, const char *value)
{
    int size = 0;
    
	if (NULL != node->value)
	{
		free(node->value);
        node->value = NULL;
	}

    if(NULL == value)
    {
        return 0;
    }

    size = strlen(value) + 1;
    
	node->value = (char *)calloc(1, size);
    if(NULL == node->value)
    {
        return -1;
    }

    snprintf(node->value, size, "%s", value);
    return 0;
}

void pub_xml_delnode_s(sw_xmlnode_t *node)
{
	sw_xmlnode_t	*parent = NULL;
	sw_xmlnode_t	*next = NULL;
	sw_xmlnode_t	*cure = NULL;
	sw_xmlnode_t	*pre = NULL;

	if (node == NULL)
	{
		return;
	}

	parent = node->parent;
	next = node->next;
	cure = parent->firstchild;
	pub_xml_delnode(node->firstchild);

	while (1)
	{
		if (cure == node)
		{
			if (cure == parent->firstchild)
			{
				parent->firstchild = next;
			}
			else
			{
				pre->next = next;
			}
			break;
		}
		pre = cure;
		cure = cure->next;
	}

	if (node->name != NULL)
	{
		free(node->name);
		node->name = NULL;
	}

	if (node->value != NULL)
	{
		free( node->value);
		node->value = NULL;
	}
	free( node);
	node = NULL;
}


#endif /*__XML_STACK__*/
