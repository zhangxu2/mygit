#include "pub_xsd.h"
#include "pub_regex.h"

static sw_int_t xsd_tree_check(sw_xmltree_t *xml_req,char *sFilePath);
static sw_int_t xsd_apatoi(char *s,int length);
static sw_int_t xsd_node_check(sw_xmltree_t *xml,sw_xmlnode_t *pnode,sw_xmltree_t *xsd,char *type);
static sw_int_t xsd_check(sw_xmltree_t *xml,sw_xmltree_t *xsd);

static sw_int_t xsd_tree_check(sw_xmltree_t *xml_req,char *sFilePath)
{
	char sBuf[128];
	sw_xmltree_t *xsd;
	int iRc;

	xsd = NULL;
	iRc = -1;
	memset(sBuf,0x00,sizeof(sBuf));

	if(xml_req==NULL || sFilePath==NULL || strlen(sFilePath)==0)
	{
		return SW_ERR;
	}

	xsd = pub_xml_crtree(sFilePath);
	if(xsd == NULL)
	{
		return SW_ERR;
	}
	else
	{
		iRc = -1;
		iRc = xsd_check(xml_req,xsd);
		if(iRc<0)
		{
			pub_xml_deltree(xsd);
			xsd=NULL;
			return SW_ERR;
		}
	}
	pub_xml_deltree(xsd);
	xsd=NULL;
	return SW_OK;
}
static sw_int_t xsd_apatoi(char *s,int length)
{
	char buf[32];
	int i;
	memset(buf,'\0',sizeof(buf));
	for(i=0;i<length;i++){
		if(s[i]>='0' && s[i]<='9'){
			buf[i]=s[i];
		}else{
			return(SW_ERR);
		}
	}
	return(atoi(buf));

}


static sw_int_t xsd_node_check(sw_xmltree_t *xml,sw_xmlnode_t *pnode,sw_xmltree_t *xsd,char *type)
{
	size_t	len = 0;
	sw_xmlnode_t *xnode1,*xnode2,*xnode3,*xnode4;
	sw_xmlnode_t *pnode1;
	sw_xmlnode_t *pnode_bak,*xnode_bak;
	char *sBaseType;
	pnode_bak=xml->current;
	xnode_bak=xsd->current;
	/****先查找简单类型****/
	xml->current=pnode;
	xnode1=pub_xml_locnode(xsd,".xs:schema.xs:simpleType");
	while(xnode1!=NULL){
		xsd->current=xnode1;
		xnode2=pub_xml_locnode(xsd,"name");
		if(xnode2==NULL){
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_ERR);
		}
		if(pub_str_strcmp(xnode2->value,type)==0){
			break;
		}
		xnode1=xnode1->next;
	}
	if(xnode1==NULL){
		/****再查找复杂类型****/
		xnode1=pub_xml_locnode(xsd,".xs:schema.xs:complexType");
		while(xnode1!=NULL){
			xsd->current=xnode1;
			xnode2=pub_xml_locnode(xsd,"name");
			if(xnode2==NULL){
				xml->current=pnode_bak;
				xsd->current=xnode_bak;
				return(SW_ERR);
			}
			if(pub_str_strcmp(xnode2->value,type)==0){
				break;
			}
			xnode1=xnode1->next;
		}
	}
	if(xnode1==NULL){
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	if(pub_str_strcmp(xnode1->name,"xs:simpleType")==0){
		/**是简单类型**/
		xnode2=pub_xml_locnode(xsd,"xs:restriction");
		if(xnode2==NULL){
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_ERR);
		}
		xsd->current=xnode2;
		xnode3=pub_xml_locnode(xsd,"base");
		if(xnode3==NULL){
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_ERR);
		}
		sBaseType=xnode3->value;
		xnode3=pub_xml_locnode(xsd,"xs:enumeration");
		if(xnode3!=NULL){
			/****枚举型****/
			while(xnode3!=NULL){
				xsd->current=xnode3;
				xnode4=pub_xml_locnode(xsd,"value");
				if(xnode4==NULL){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				if(pnode->value!=NULL && pub_str_strcmp(pnode->value,xnode4->value)==0){
					/****简单类型枚举类型匹配****/
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_OK);
				}
				xnode3=xnode3->next;
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_ERR);
		}
		if(pub_str_strcmp(sBaseType,"xs:decimal")==0){
			/****数字型****/
			xnode3=pub_xml_locnode(xsd,"xs:totalDigits.value");
			if(xnode3!=NULL && pnode -> value != NULL ){
				len = atoi(xnode3->value);
				if(strlen(pnode->value)>len){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xnode3=pub_xml_locnode(xsd,"xs:fractionDigits.value");
			if(xnode3!=NULL && pnode -> value != NULL ){
				int cnt=0;
				int id=0;
				while(pnode->value[id]!='\0'){
					if(pnode->value[id]=='.'){
						break;
					}
					id++;
				}
				if(pnode->value[id]=='.'){
					id++;
					while(pnode->value[id]!='\0'){
						cnt++;
						id++;
					}
				}
				if(cnt!=atoi(xnode3->value)){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		if(pub_str_strcmp(sBaseType,"xs:string")==0){
			xnode3=pub_xml_locnode(xsd,"xs:minLength.value");
			if(xnode3!=NULL && pnode -> value != NULL ){
				len = atoi(xnode3->value);	
				if(strlen(pnode->value)<len){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xnode3=pub_xml_locnode(xsd,"xs:maxLength.value");
			if(xnode3!=NULL && pnode -> value != NULL ){
				len = atoi(xnode3->value);
				if(strlen(pnode->value)>len){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xnode3=pub_xml_locnode(xsd,"xs:pattern.value");
			if(xnode3!=NULL && pnode -> value != NULL ){
				if (pub_regex_match(pnode->value, xnode3->value)){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		if(memcmp(sBaseType,"xs:date",7)==0){
			if(xsd_apatoi(pnode->value,4)>3000 ||xsd_apatoi(pnode->value,4)<1900){
				xml->current=pnode_bak;
				xsd->current=xnode_bak;
				return(SW_ERR);
			}
			if(xsd_apatoi(pnode->value+5,2)>12 ||
					xsd_apatoi(pnode->value+5,2)<1){
				xml->current=pnode_bak;
				xsd->current=xnode_bak;
				return(SW_ERR);
			}
			if(xsd_apatoi(pnode->value+8,2)>31 ||
					xsd_apatoi(pnode->value+8,2)<1){
				xml->current=pnode_bak;
				xsd->current=xnode_bak;
				return(SW_ERR);
			}
			if(pub_str_strcmp(sBaseType,"xs:dateTime")==0){
				if(xsd_apatoi(pnode->value+11,2)>24 ||
						xsd_apatoi(pnode->value+11,2)<0){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				if(xsd_apatoi(pnode->value+14,2)>60 ||
						xsd_apatoi(pnode->value+14,2)<0){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				if(xsd_apatoi(pnode->value+17,2)>60 ||
						xsd_apatoi(pnode->value+17,2)<0){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				if(pnode->value[4]!='-' ||pnode->value[7]!='-'||
						pnode->value[10]!='T' ||pnode->value[13]!=':'||
						pnode->value[16]!=':' ){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_OK);
	}
	else
	{	/**是复杂类型**/
		xnode2=pub_xml_locnode(xsd,"xs:sequence.xs:element");
		if(xnode2!=NULL){
			while(xnode2!=NULL){
				int nMinOccurs=1;
				xsd->current=xnode2;
				xnode3=pub_xml_locnode(xsd,"minOccurs");
				if(xnode3!=NULL){
					nMinOccurs=atoi(xnode3->value);
				}
				xnode3=pub_xml_locnode(xsd,"name");
				if(xnode3==NULL){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				xnode4=pub_xml_locnode(xsd,"type");
				if(xnode4==NULL){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				pnode1=pub_xml_locnode(xml,xnode3->value);
				if(pnode1==NULL && nMinOccurs>0){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);

				}
				if(pnode1!=NULL){
					if(xsd_node_check(xml,pnode1,xsd,xnode4->value)){
						xml->current=pnode_bak;
						xsd->current=xnode_bak;
						return(SW_ERR);
					}
				}
				xnode2=xnode2->next;
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		xnode2=pub_xml_locnode(xsd,"xs:sequence.xs:choice.xs:element");
		if(xnode2!=NULL){
			while(xnode2!=NULL){
				xsd->current=xnode2;
				xnode3=pub_xml_locnode(xsd,"name");
				if(xnode3==NULL){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				xnode4=pub_xml_locnode(xsd,"type");
				if(xnode4==NULL){
					xml->current=pnode_bak;
					xsd->current=xnode_bak;
					return(SW_ERR);
				}
				pnode1=pub_xml_locnode(xml,xnode3->value);
				if(pnode1==NULL){
					xnode2=xnode2->next;
					continue;
				}
				if(pnode1!=NULL){
					if(xsd_node_check(xml,pnode1,xsd,xnode4->value)){
						xml->current=pnode_bak;
						xsd->current=xnode_bak;
						return(SW_ERR);
					}
				}
				xnode2=xnode2->next;
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		xnode2=pub_xml_locnode(xsd,"xs:simpleContent.xs:extension.base");
		if(xnode2!=NULL){
			if(xsd_node_check(xml,pnode,xsd,xnode2->value)){
				xml->current=pnode_bak;
				xsd->current=xnode_bak;
				return(SW_ERR);
			}
			xnode2=pub_xml_locnode(xsd,"xs:simpleContent.xs:extension.xs:attribute");
			while(xnode2!=NULL){
				sw_xmlnode_t *pname,*ptype;
				xsd->current=xnode2;
				pnode1=NULL;
				pname=pub_xml_locnode(xsd,"name");
				ptype=pub_xml_locnode(xsd,"type");
				xnode3=pub_xml_locnode(xsd,"use");
				if(pname!=NULL){
					pnode1=pub_xml_locnode(xml,pname->value);
				}
				if(pub_str_strcmp(xnode3->value,"required")==0||pnode1!=NULL){
					if(pname==NULL || ptype==NULL){
						xml->current=pnode_bak;
						xsd->current=xnode_bak;
						return(SW_ERR);
					}
					if(pnode1==NULL){
						xml->current=pnode_bak;
						xsd->current=xnode_bak;
						return(SW_ERR);
					}
					if(xsd_node_check(xml,pnode1,xsd,ptype->value)){
						xml->current=pnode_bak;;
						xsd->current=xnode_bak;
						return(SW_ERR);
					}
				}
				xnode2=xnode2->next;
			}
			xml->current=pnode_bak;
			xsd->current=xnode_bak;
			return(SW_OK);
		}
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	xml->current=pnode_bak;;
	xsd->current=xnode_bak;
	return(SW_OK);

}

static sw_int_t xsd_check(sw_xmltree_t *xml,sw_xmltree_t *xsd)
{
	sw_xmlnode_t *pnode1;
	sw_xmlnode_t *xnode1;
	char sName[128];
	char *sType;
	sw_xmlnode_t *pnode_bak,*xnode_bak;
	pnode_bak=xml->current;
	xnode_bak=xsd->current;
	xnode1=pub_xml_locnode(xsd,".xs:schema.xs:element.name");
	if(xnode1==NULL){
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	sprintf(sName,".%s",xnode1->value);
	pnode1=pub_xml_locnode(xml,sName);
	if(pnode1==NULL){
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	xnode1=pub_xml_locnode(xsd,".xs:schema.xs:element.type");
	if(xnode1==NULL){
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	sType=xnode1->value;
	if(!xsd_node_check(xml,pnode1,xsd,sType)){
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_OK);
	}else{
		xml->current=pnode_bak;
		xsd->current=xnode_bak;
		return(SW_ERR);
	}
	return(SW_OK);
}


sw_int_t pub_xsd_check(char *sFileName, char *sBuffer)
{
	int	iRc = 0;
	sw_xmltree_t *xml_req = NULL;

	if(sFileName==NULL || strlen(sFileName)==0 
		|| sBuffer==NULL || strlen(sBuffer)==0)
	{
		return SW_ERR;
	}

#if 0
	sPtr = NULL;
	sPtr = strstr(head,"<?xml");
	if (sPtr == NULL)
	{
		return SW_ERR;
	}
	xml_req = pub_xml_unpack_ext(sPtr,0);
	if(xml_req==NULL)
	{
		return SW_ERR;
	}
#endif 

	if (!pub_file_exist(sBuffer))
	{
		pub_log_error("[%s][%d]xsd_check name[%d] not found", __FILE__, __LINE__,sBuffer);
		return SW_ERROR;
	}

	if (!pub_file_exist(sFileName))
	{
		pub_log_error("[%s][%d]xsd_check name[%d] not found", __FILE__, __LINE__,sFileName);
		return SW_ERROR;
	}

	xml_req = pub_xml_crtree(sBuffer);
        if(xml_req==NULL)
        {
                return SW_ERR;
        }

	iRc = xsd_tree_check(xml_req,sFileName);
	if( iRc<0 )
	{
		pub_log_error("[%s][%d]xsd check error file[%s],xsd[%s]",__FILE__,__LINE__,sBuffer,sFileName);
		pub_xml_deltree(xml_req);
		xml_req=NULL;
		return SW_ERR;
	}
	
	pub_xml_deltree(xml_req);
	xml_req=NULL;
	return SW_OK;
}

/*int test()
{
	int iRc;
	int iLen;
	char sBuffer[128*1024];
	char sFileName[256];
	FILE  *fp=NULL;
	struct stat  tFileStat;

	memset(sFileName,0x00,sizeof(sFileName));
	memset(sBuffer,0x00,sizeof(sBuffer));

	sprintf(sFileName,"/home/cspt/usr/hg/CHECKPKG/body.xml");
	if( stat( sFileName, &tFileStat ) )
	{
		printf("file [%s] stat error!!errno=[%d][%s] pid=[%d]\n",
		sFileName, errno, strerror(errno),getpid());
		return -1;
	}

	iLen=tFileStat.st_size;
	fp=NULL;
	fp=fopen(sFileName,"r");
	if(fp==NULL)
	{
		printf("open file %s error!!!errno=[%d][%s] pid=[%d]\n",sFileName,errno,strerror(errno),getpid());
		return -1;
	}
	iRc=fread(sBuffer,1,iLen,fp);
	printf("[%s]\n",sBuffer);
	fclose(fp);
	char *path = "/home/cspt/usr/hg/CHECKPKG/filecheck.xsd";
	iRc = iCheckPkgLegal(path, sBuffer);
	if(iRc<0)
	{
		printf("格式错误\n");
		return -1;
	}
	printf("格式正确\n");
	return 0;
}*/

