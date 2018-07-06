/*********************************************************************
 *** 版 本 号: v2.0
 *** 程序作者: Shi Shenghui
 *** 生成日期: 2008-01-29
 *** 所属模块: 公共模块
 *** 程序名称: pub_xml.c
 *** 程序作用: xml格式文件处理公共函数,xml解析器
 *** 函数列表:
 ***		xml_initnode		树节点初始化
 ***		pub_xml_readline		读取一行数据
 ***		pub_xml_line_getvalue		从一xml行中得到值部分，并返回值之后下一个字符的位置
 ***		pub_xml_line_getname		从一行中得到名称部分，并返回名称之后下一个字符的位置
 ***		pub_xml_line_getattr		从一xml行中得到属性部分，并返回属性之后下一个字符的位置	
 ***		pub_xml_setcurrent		设置输入节点为当前xml树节点
 ***		pub_xml_prt			打印当前xml树节点及以下全部节点
 ***		pub_xml_locnode			定位某个结点
 ***		pub_xml_crtree			建树
 ***		pub_xml_crnode			创建指定标签数支节点
 ***		pub_xml_dealvalue		处理当前值
 ***		pub_xml_delnode			清除当前节点
 ***		pub_xml_unpack			组建xml树
 ***		pub_xml_pack			存xml树到文件
 ***		xml_savenode		保存当前节点到文件,递归函数
 ***		pub_xml_get_nodevalue		得到当前标签的子标签的属性或子标签的值
 *** 使用注意:
 *** 修改记录:
 *** 	修改作者:liteng
 *** 	修改时间:2011-11-29
 *** 	修改内容:调整了代码格式，重新定义了函数接口
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
 
 /****转换了XML关键字的字符串拷贝****/
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
 *** 函数名称: xml_initnode
 *** 函数功能: 树节点初始化
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-01-29 15:32
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: pstXmlNode		树节点指针
 *** 输出参数: 无
 *** 返 回 值: void
 *** 注意事项: 
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
 *** 函数名称: xml_readline
 *** 函数功能: 读取一行数据
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 15:36
 *** 调用函数: 
 *** 	函数1: xml_readline_a
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: fp		文件指针
 *** 	参数2: line		当前行数据指针
 *** 	参数3: readbuf	数据缓存区
 *** 	参数4: pos	下一个位置指针
 *** 输出参数: 无
 *** 返 回 值: void
 *** 注意事项: 
 ******************************************************************************/
static sw_int_t xml_readline(sw_xmlmem_t *fp, sw_buf_t *buf, char *readbuf,int *pos,int type)
{
	int	idx = 1;
	char	*line = NULL;
	/*读一行*/	
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
 *** 函数名称: xml_readline_a
 *** 函数功能: 读取一行数据扩展函数
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 15:36
 *** 调用函数: 
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: fp		文件指针
 *** 	参数2: line		当前行数据指针
 *** 	参数3: readbuf	数据缓存区
 *** 	参数4: pos	下一个位置指针
 *** 输出参数: 无
 *** 返 回 值: void
 *** 注意事项: 
  ******************************************************************************/
static sw_int_t xml_readline_a(sw_xmlmem_t *fp, sw_buf_t *buf, char *readbuf,int *pos,int type)
{	/*读一行*/
	int	pos1;
	int	bakpos;		/*备份之前的iPos1位置，当注释走完后恢复*/
	int	readline;
	char	cChar;
	char	cBak;
	char	cTmp;
	char	cCh;
	int	iBinQuot;
	int	docflag = 0;
	char	*line = NULL;
	
	/*在读缓冲中读出由一对<>封起来的部分,缓冲区读完要重新读文件*/
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
			{	/*不要最后不完整的行*/
				line[0] = '\0';
				break;
			}
			cChar = readbuf[(*pos)++];
		}
		if (!iBinQuot)
		{	/*在双引号外的部分，把所有空白都化作一个空格*/
			if ((cChar == 13 || cChar == 10 || cChar == '\t') && (pos1 == 0 || line[0] == '<'))
			{
				cChar = ' ';
			}
			if (cChar == ' ')
			{	/*等号之后也不要空格*/
				if (pos1 == 0 || (line[pos1-1] == ' ' && line[0] == '<'))
				{
					continue;
				}
				
				/*** 只删除属性中等号两端的空格 ***/
				if (line[pos1 - 1] == '=' && line[0] == '<')
				{
					continue;
				}
			}
			if (cChar == '"' && (line[0] == '<' || pos1 == 1))/*是双引号则改变引号标志*/
			{
				iBinQuot = 1;
			}
			/*** 只删除属性中等号两端的空格 ***/
			if ((cChar == '=' && line[0] == '<') || (cChar == '/' && line[0] == '<')) /*若为等号，则删除之前的所有空格*/
			{
				if ((pos1>0)&&(line[pos1-1] == ' '))
				{
					pos1--;
				}
			}
			if (cChar == '-')
			{	/*删除注释*/
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
				/*删除?注释*/
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

			/*正常可以写入psLine的字符*/
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
			if (cChar == '"')/*是双引号则改变引号标志*/
			{
				iBinQuot = 0;
			}
			
		}/* end if iBinQuot condition */
	}/* end while */

	return SW_OK;
}

/******************************************************************************
 *** 函数名称: pub_xml_line_getvalue
 *** 函数功能: 从一xml行中得到value部分，并返回value之后下一个字符的位置
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 15:42
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: line		当前行数据缓冲区
 *** 输出参数: 无
 *** 	参数1: pvalue		当前行值部分
 *** 返 回 值: value之后下一个字符的位置
 *** 注意事项: 
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
 *** 函数名称: pub_xml_line_getname
 *** 函数功能: 从一xml行中得到名称部分，并返回名称之后下一个字符的位置
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 15:47
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: line		当前行数据缓冲区
 ***	参数2: iPos		当前位置
 *** 输出参数: 
 ***    参数1: name		当前行名称部分
 *** 返 回 值: 返回名称之后下一个字符的位置
 *** 注意事项: 
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
 *** 函数名称: pub_xml_line_getattr
 *** 函数功能: 从一xml行中得到attrib部分，并返回attrib之后下一个字符的位置
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 15:52
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: line		当前行缓冲区
 ***	参数2: iPos		当前位置
 *** 输出参数: 
 *** 	参数1: pattrname	属性名部分
 ***	参数2: psAttrValue	属性值部分
 *** 返 回 值: 返回属性之后下一个字符的位置
 *** 注意事项: 
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

	iPos2 = iPos + 1;/*查找=位置*/
	while(line[iPos2] != '\0' && line[iPos2] != '=')
	{
		iPos2++;
	}
	
	/*填写属性的名称*/
	memcpy(pattrname,line + iPos + 1,iPos2 - iPos - 1);
	pattrname[iPos2-iPos-1] = '\0';
	
	/*填写属性的值*/
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
 *** 函数名称: xml_dealvalue
 *** 函数功能: 处理当前值
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 16:39
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: psValue		当前要处理的值指针
 *** 输出参数: 无
 *** 返 回 值: 处理结果
 *** 注意事项: 
 ******************************************************************************/
static char * xml_dealvalue(char *value)
{
	int idx;
	int j;
	int iLen;

	/*删除尾部空格及双引号*/
	idx = strlen(value) - 1;
	/*** 不能删除节点尾部的空格 ***/
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
	
	/*特殊字符 < > "的处理"*/
	idx = 0;
	iLen = strlen(value);
	while(value[idx] != '\0')
	{
		if (value[idx] == '&')
		{
			/******************************* 
				&lt;    <    小于
				&gt;	>    大于
				&amp;   &    和号
				&apos;  '    单引号
				&quot;  "    引号
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
 *** 函数名称: xml_savenode
 *** 函数功能: 保存当前节点到文件,递归函数
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 18:39
 *** 调用函数: 
 *** 	函数1: xml_savenode
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: pstCurNode		当前节点指针
 *** 	参数2: iLevel			当前节点层次级别
 *** 	参数3: pstCurNode		文件指针
 *** 输出参数: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
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
		/*不能以属性做主节点*/
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
			/*有子节点*/
			break;
		}
		pstNode2 = pstNode2->next;
	}

	if (pstNode2 != NULL)
	{
		/*有子节点*/
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
		/*无子节点*/
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
			/*** 没有属性 ***/
			xmlmem_write(fpt, type, "></%s>\n", pstNode1->name);
		}
		else
		{
			/*** 有属性 ***/
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
	
	/*构造根结点*/
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
	root->node_type = SW_NODE_ROOT;/*为模拟oracle 9i解析器用*/

	/*构造分支结点*/
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
		{/**有value**/
			if (flg == SW_S_BROTHER)
			{	/*期待兄弟*/
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
	 	{	/*没有value项*/
			if (name[0] != '/')
			{	/*是开始行*/
				node = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
				CHECKPTR(node);
				xml_initnode(node);
				node->node_type = SW_NODE_ELEMENT;/*为模拟oracle9i解析器用*/
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
				{	/*有属性*/
					node = (sw_xmlnode_t *)malloc(sizeof( sw_xmlnode_t));
					CHECKPTR(node);
					xml_initnode(node);
					node->node_type = SW_NODE_ATTRIB;/*为模拟oracle 9i解析器用*/
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
						node->node_type = SW_NODE_ATTRIB;/*为模拟oracle 9i解析器用*/
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
					{	/*有结束标志*/
						cur = cur->parent;
						flg = SW_S_BROTHER;
						continue;
					}
					else
					{	/*没有结束标志*/
						flg = SW_S_BROTHER;
						continue;
					}
				}
				else
				{	/*没有属性*/
					flg = SW_S_CHILD;
					continue;
				}
			}
			else
			{	
				/*是结束行*/
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
			}/*end 开始行*/
		}/*end if else 没有value项*/
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
  *** 函数名称: pub_xml_crtree
  *** 函数功能: 根据在文件中读入的一行内容以及当前的结点，在树中插入一个结点
  ***		 flg标志新结点与当前结点的关系，SW_S_CHILD为孩子,SW_S_BROTHER为兄弟
  ***		 返回值为新结点地址（如果读入的行是/>形式的回溯行，则返回回溯到的结点)
  ***		 flg在返回时被设置成下一行与当前行的关系
  *** 函数作者: Shi Shenghui
  *** 生成日期: 2008-1-29 16:19
  *** 调用函数: 无	
  *** 访问的表: 无
  *** 修改的表: 无
  *** 输入参数: 
  ***	 参数1: psFileName		 文件路径指针
  *** 输出参数: 无
  *** 返 回 值: 0:成功，非0:失败
  *** 注意事项: 
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
 *** 函数名称: pub_xml_locnode
 *** 函数功能: 定位某个结点,相对"item.name.col"或绝对如".item.name.col",
 ***		绝对路径改变当前路径，相对路径不改变当前路径
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 16:2
 *** 调用函数: 无 
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: pstXml		树结构体指针
 *** 	参数1: psLoc		查询的节点位置
 *** 输出参数: 
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
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
		/*绝对路径*/
		node = pstXml->root;
		iPos1 = 1;
		iPos2 = 1;
	}
	else
	{	
		/*相对路径*/
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

/****增加一个属性****/
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
 *** 函数名称: pub_xml_crnode
 *** 函数功能: 创建指定标签数支节点
 *** 函数作者: zhang hailu
 *** 生成日期: 2011-08-01
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: pstXml		需要建立标签的树
 *** 	参数2: name		创建节点的标签（如.NAME.FLAG 即创建FLAG支节点，
 ***					若NAME不存在同时创建NAME支节点）
 ***    参数4: flg		是否为属性标签
 *** 输出参数: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
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
		/*绝对路径*/
		iPos1 = 1;
		iPos2 = 1;
		node = pstXml->root;
	}
	else
	{	
		/*相对路径*/
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
 *** 函数名称: pub_xml_delnode
 *** 函数功能: 清除当前节点
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 16:43
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: node		当前节点指针
 *** 输出参数: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
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
 *** 函数名称: pub_xml_unpack
 *** 函数功能: 组建xml树
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 18:32
 *** 调用函数: 
 *** 	函数1: pub_xml_crtree
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: psPack		文件路径指针
 *** 输出参数: 无
 *** 返 回 值: 树结构指针
 *** 注意事项: 
 ******************************************************************************/
sw_xmltree_t * pub_xml_unpack(char *psPack)
{
	sw_xmlnode_t *root;
	sw_xmltree_t *pstXml;
	
	/*允许初始化一个空XML树*/
	if (psPack == NULL)
	{
		root = (sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t)) ;
		xml_initnode(root);
		root->name = (char *)malloc(5);
		strcpy(root->name,"ROOT");
		root->value = (char *)malloc(7);
		strcpy(root->value,"SEAGLE");
		root->node_type = SW_NODE_ROOT;/*为模拟oracle 9i解析器用*/
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
	

	/****允许初始化一个空XML树****/
	if(pack==NULL){
		proot=(sw_xmlnode_t *)malloc(sizeof(sw_xmlnode_t));
		xml_initnode(proot);
		proot->name=(char *)malloc(5);
		strcpy(proot->name,"ROOT");
		proot->value=(char *)malloc(7);
		strcpy(proot->value,"SEAGLE");
		proot->node_type=SW_NODE_ROOT;/*为模拟oracle 9i解析器用*/
		pxml=(sw_xmltree_t *)malloc(sizeof(sw_xmltree_t));
		pxml->root=proot;
		pxml->current=proot;
		return(pxml);
	}

	return pub_xml_crtree_ext(pack,iLen);

}

/******************************************************************************
 *** 函数名称: pub_xml_pack
 *** 函数功能: 存xml树到文件
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 18:35
 *** 调用函数: 
 *** 	函数1: pub_xml_pack
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: pstXml			xml树指针
 *** 	参数2: psPack			文件路径指针
 *** 输出参数: 无
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
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
		pub_log_error("%s,%d,XML树不存在\n",__FILE__,__LINE__);
		xmlmem_clear(&fp,XML_MEM_TYPE);
		return(-1);
	}
	if(pxml->root==NULL){
		pub_log_error("%s,%d,XML树不存在\n",__FILE__,__LINE__);
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

/* 返回计算的XML报文的长度 */
sw_int_t pub_xml_calc_len(sw_xmlnode_t *pnode_cur, int level)
{
	int	i = 0, length = 0, length2 = 0;
	int	have_attr = 0;
	sw_xmlnode_t *pnode1,*pnode2;
	
	if(pnode_cur==NULL){
		pub_log_error("%s,%d,XML树不存在\n",__FILE__,__LINE__);
		return(-1);
	}
	if(pnode_cur->node_type==SW_NODE_ATTRIB){
		/****不能以属性做主节点****/
		pub_log_error("%s,%d,XML树不存在\n",__FILE__,__LINE__);
		return(-1);
	}
	
	if (level == 0 && pnode_cur->firstchild == NULL)
	{
		return 0;
	}

	pnode1=pnode_cur;
	if(level>0){
		for(i=0;i<level;i++){
			/*** TAB键 ***/
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
			/****有子节点****/
			break;
		}
		pnode2=pnode2->next;
	}
	if(pnode2!=NULL)
	{
		/****有子节点****/
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
					/*** TAB键 ***/
					length += 1;
				}
			}
			/****考虑关键字的转换****/
			/****
			sprintf(fp->start+fp->length,"%s\n",pnode1->value);
			****/
			/*** VALUE\n ***/
			length += xml_strlen(pnode1->value) + 1;
		}
		if(level>0){
			for(i=0;i<level;i++){
				/*** TAB键 ***/
				length += 1;
			}
		}
		/*** </NAME>\n ***/
		length += 4 + strlen(pnode1->name);
	}else{
		/****无子节点****/
		if(pnode1->value!=NULL){
			/*** >\n ***/
	#if 0
			length += 2;
			if(level>0){
				for(i=0;i<level;i++){
					/*** TAB键 ***/
					length += 1;
				}
			}
			/****考虑关键字的转换****/
			/****
			sprintf(fp->start+fp->length,"%s\n",pnode1->value);
			****/
			length += strlen(pnode1->value) + 1;
			if(level>0){
				for(i=0;i<level;i++){
					/*** TAB键 ***/
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
 *** 函数名称: pub_xml_prt
 *** 函数功能: 打印当前xml树节点及以下全部节点
 *** 函数作者: Shi Shenghui
 *** 生成日期: 2008-1-29 16:0
 *** 调用函数: 无
 *** 访问的表: 无
 *** 修改的表: 无
 *** 输入参数: 
 *** 	参数1: node		当前节点指针
 *** 输出参数: 无
 *** 返 回 值: void
 *** 注意事项: 		
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
 *** 函数名称: pub_xml_get_nodevalue
 *** 函数功能: 得到当前标签的子标签的属性或子标签的值
 *** 函数作者: liteng
 *** 生成日期: 2011-11-29 18:45
 *** 调用函数: 
 *** 	函数1: pub_xml_locnode
 *** 访问的表: 
 *** 修改的表: 
 *** 输入参数: 	
	 *** 	参数1: pstXml 		xml树结构指针
 *** 	参数2: psMark 		当前标签的子标签的名字
 ***    参数3: psNodeName   	当前标签的子标签的属性或子标签的名字
 ***    参数4: psMark   	当前标签的子标签的名字
 *** 输出参数: 
 *** 	参数1: sValue		当前标签的子标签的属性或子标签的值
 *** 返 回 值: 0:成功，非0:失败
 *** 注意事项: 
 *** 使用说明：
 ***<global>
 ***	<infoserver name="swcomm" maxnum = "3" minmun ="10" >
 ***		<out>mytest hello</out>
 ***	</infoserver>
 ***</global>
 ***当前标签<global>，子标签<infoserver>，子标签的属性name，子标签的子标签<out>
 ***输入参数：psMark="infoserver",psNodeName="name",输出：sValue="swcomm"
 ***输入参数：psMark="infoserver",psNodeName="out",输出：sValue="mytest hello"
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
 **函数名称: pub_xml_set_value
 **功    能: 设置节点值
 **输入参数:
 **     node: XML节点
 **     value: 节点值
 **输出参数:
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.12 #
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
