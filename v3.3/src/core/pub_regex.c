
#include "pub_regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>



#define SPCHAR_NUMBER   1
#define SPCHAR_LOWCHAR  2
#define SPCHAR_HIGHCHAR 3
#define SPCHAR_ALLCHAR  4
#define SPCHAR_DOT      5
#define SPCHAR_STAR     6
#define SPCHAR_PLUS     7
#define SPCHAR_QUESTION 8
#define SPCHAR_SLASH    10 
#define SPCHAR_LBROKE    11 
#define SPCHAR_RBROKE    12 
#define MACHAR_LBROKE     '{' 
#define MACHAR_RBROKE     '}' 

int iGetPatternUnit(char *pattern,char *caUnit,char *caPattern,int *offset){	
	int i,j;
	if(pattern!=NULL){
		i=j=0;
		while(pattern[i]!='\0'){
			if(pattern[i]=='\\' && pattern[i+1]=='d'){
				caPattern[j]=SPCHAR_NUMBER;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='a'){
				caPattern[j]=SPCHAR_LOWCHAR;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='A'){
				caPattern[j]=SPCHAR_HIGHCHAR;
				i+=2;
				j++;
			}else if(pattern[i]=='.' ){
				caPattern[j]=SPCHAR_ALLCHAR;
				i++;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='.'){
				caPattern[j]=SPCHAR_DOT;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='*'){
				caPattern[j]=SPCHAR_STAR;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='+'){
				caPattern[j]=SPCHAR_PLUS;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='?'){
				caPattern[j]=SPCHAR_QUESTION;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]=='\\'){
				caPattern[j]=SPCHAR_SLASH;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]==MACHAR_LBROKE){
				caPattern[j]=SPCHAR_LBROKE;
				i+=2;
				j++;
			}else if(pattern[i]=='\\' && pattern[i+1]==MACHAR_RBROKE){
				caPattern[j]=SPCHAR_RBROKE;
				i+=2;
				j++;
			}else{
				caPattern[j]=pattern[i];
				i++;
				j++;
			}
		}	
		caPattern[j]='\0';
		*offset=0;
	}
	caUnit[0]='\0';
	j=0;
	if(caPattern[*offset]!='\0'){
		switch(caPattern[*offset]){
		case '[':
			while(caPattern[*offset]!=']'){
				if(caPattern[*offset]=='\0'){
					caUnit[0]='\0';
					return(-1);	
				}
				caUnit[j]=caPattern[*offset];
				j++;
				(*offset)++;
			}
			caUnit[j]=caPattern[*offset];
			j++;
			(*offset)++;
			caUnit[j]='\0';
			break; 
		default:
			caUnit[j]=caPattern[*offset];
			j++;
			(*offset)++;
			caUnit[j]='\0';
			
		}
	}
	switch(caPattern[*offset]){
	case '?':
	case '+':
	case '*':
		caUnit[j]=caPattern[*offset];
		j++;
		(*offset)++;
		caUnit[j]='\0';
		break;		
	case MACHAR_LBROKE:
		while(caPattern[*offset]!=MACHAR_RBROKE && caPattern[*offset]!='\0'){
			caUnit[j]=caPattern[*offset];
			j++;
			(*offset)++;
		}
		caUnit[j++]=MACHAR_RBROKE;
		caUnit[j]='\0';
		(*offset)++;
		break;
	}
	return(0);
}
/****比较一个字符是否匹配****/
/****返回:0匹配,-1不匹配**/
int nCharMatch(char ch,char pattern){
	switch(pattern){
	case SPCHAR_NUMBER:
		if(ch>='0' && ch<='9'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_LOWCHAR:
		if(ch>='a' && ch<='z'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_HIGHCHAR:
		if(ch>='A' && ch<='Z'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_ALLCHAR:
		return(0);
	case SPCHAR_DOT:
		if(ch=='.'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_STAR:
		if(ch=='*'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_PLUS:
		if(ch=='+'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_QUESTION:
		if(ch=='?'){
			return(0);
		}else{
			return(-1);
		}
	case SPCHAR_SLASH:
		if(ch=='\\'){
			return(0);
		}else{
			return(-1);
		}
	default:
		if(pattern==ch){
			return(0);
		}else{
			return(-1);
		}
	}
	return(-1);
}
/****比较字符串是否匹配****/
/****返回值:0匹配,-1不匹配****/
int nStringMatch_a(char *str,char *pattern){
	int i=0;
	int j=0;
	int len=0;
	int cnt=0;
	int offset=0;
	int ret=-1;
	int FirstMatch=0;
	int	alloc_size = 0;
	char *pcInPattern;
	char *caPattern;

	len = strlen(pattern);
	alloc_size = len + 1;
	pcInPattern = (char *)calloc(1, alloc_size);
	caPattern = (char *)calloc(1, alloc_size);
	len = 0;
	iGetPatternUnit(pattern,pcInPattern,caPattern,&offset);
	FirstMatch=1;
	if(pcInPattern=='\0'){
		free(caPattern);
		free(pcInPattern);		
		return(-1);
	}
	while(pcInPattern[0]!='\0'){
		len=strlen(pcInPattern);
		if(str[i]=='\0'){
			while(pcInPattern[0]!='\0'){
				if(!FirstMatch){
					if(pcInPattern[len-1]!='+' && pcInPattern[len-1]!='*' && pcInPattern[len-
1]!=MACHAR_RBROKE){
						free(caPattern);
						free(pcInPattern);
						return(-1);	
					}
				}else{
					if(pcInPattern[len-1]!='?' && pcInPattern[len-1]!='*' && pcInPattern[len-
1]!=MACHAR_RBROKE){
						free(caPattern);
						free(pcInPattern);
						return(-1);
					}
				}
				if(pcInPattern[len-1]==MACHAR_RBROKE){
					int nMinCnt,nMaxCnt;
					int nIndex;
					int nNextFlag=0;
					nMinCnt=nMaxCnt=0;
					nIndex=len-1;
					while(pcInPattern[nIndex]!=MACHAR_RBROKE){
						if(pcInPattern[nIndex]>='0' && pcInPattern[nIndex]<='9'){
							if(!nNextFlag){
								nMinCnt=nMinCnt*10;
								nMinCnt+=pcInPattern[nIndex]-'0';
							}else{
								nMaxCnt=nMaxCnt*10;
								nMaxCnt+=pcInPattern[nIndex]-'0';
							}	
						}else if(pcInPattern[nIndex]==','){
							nNextFlag=1;
						}else{
							free(caPattern);
							free(pcInPattern);
							return(-1);
						}
						nIndex++;
					}
					if(!nNextFlag){
						nMaxCnt=nMinCnt;
					}
					if(cnt>=nMinCnt && cnt<=nMaxCnt){
						cnt=0;
					}else{
						free(caPattern);
						free(pcInPattern);
						return(-1);
					}
				}
				memset(pcInPattern,0x00,alloc_size);
				iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
				if(pcInPattern=='\0'){
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}
				len=strlen(pcInPattern);
			}
			free(caPattern);
			free(pcInPattern);
			return(ret);
		}
		if(pcInPattern[0]=='['){
			j=1;
			while(j<len){
				while(pcInPattern[j]=='[' || pcInPattern[j]=='|'){
					j++;
				}
				if(pcInPattern[j]=='\0'){
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}
				if(pcInPattern[j]==']'){
					break;
				}
				ret=nCharMatch(str[i],pcInPattern[j]);
				if(ret==0){
					break;
				}
				j++;
			}
		}else{
			ret=nCharMatch(str[i],pcInPattern[0]);
		}
		cnt++;
		if(pcInPattern[len-1]=='?'){
			if(ret==0){
				/***?已经匹配一次****/
				cnt=0;
				i++;
				memset(pcInPattern,0x00,alloc_size);
				iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
				if(pcInPattern=='\0'){
					free(caPattern);
					free(pcInPattern);
					return(-1);
		
				}
				FirstMatch=1;
			}else{
				/***?未能匹配一次****/
				cnt=0;
				memset(pcInPattern,0x00,alloc_size);
				iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
				if(pcInPattern=='\0'){
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}
				FirstMatch=1;
			}
			continue;
		}else if(pcInPattern[len-1]=='+'){
			if(ret==0){
				/***+匹配****/
				i++;
				FirstMatch=0;
			}else{
				/***+不匹配****/
				if(cnt<=1){
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}else{
					cnt=0;
					memset(pcInPattern,0x00,alloc_size);
					iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
					if(pcInPattern=='\0'){
						free(caPattern);
						free(pcInPattern);
						return(-1);
		
					}
					FirstMatch=1;					
				}
			}
			continue;
		}else if(pcInPattern[len-1]=='*'){
			if(ret==0){
				/***+匹配****/
				cnt=0;
				i++;
				FirstMatch=0;
			}else{
				/***+不匹配****/
				cnt=0;
				memset(pcInPattern,0x00,alloc_size);
				iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
				if(pcInPattern=='\0'){
					free(caPattern);
					free(pcInPattern);
					return(-1);
		
				}
				FirstMatch=1;
				
			}
			continue;
		}else if(pcInPattern[len-1]==MACHAR_RBROKE){
			int nMinCnt,nMaxCnt;
			int nIndex;
			int nNextFlag=0;
			nMinCnt=nMaxCnt=0;
			nIndex=len-1;
			while(nIndex>0){
				if(pcInPattern[nIndex]==MACHAR_LBROKE){
					break;
				}
				nIndex--;
			}
			nIndex++;
			while(pcInPattern[nIndex]!=MACHAR_RBROKE){
				if(pcInPattern[nIndex]>='0' && pcInPattern[nIndex]<='9'){
					if(!nNextFlag){
						nMinCnt=nMinCnt*10;
						nMinCnt+=pcInPattern[nIndex]-'0';
					}else{
						nMaxCnt=nMaxCnt*10;
						nMaxCnt+=pcInPattern[nIndex]-'0';
					}	
				}else if(pcInPattern[nIndex]==','){
					nNextFlag=1;
				}else{
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}
				nIndex++;
			}
			if(!nNextFlag){
				nMaxCnt=nMinCnt;
			}
			if(ret==0){
				/***+匹配****/
				i++;
				FirstMatch=0;
				if(cnt>=nMaxCnt){
					cnt=0;
					memset(pcInPattern,0x00,alloc_size);
					iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
					if(pcInPattern=='\0'){
						free(caPattern);
						free(pcInPattern);
						return(-1);
					}
					FirstMatch=1;
				}
			}else{
				/***+不匹配****/
				if(cnt>nMinCnt && cnt <=nMaxCnt+1){
					cnt=0;
					memset(pcInPattern,0x00,alloc_size);
					iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
					if(pcInPattern=='\0'){
						free(caPattern);
						free(pcInPattern);
						return(-1);
					}
					FirstMatch=1;
				}else{
					free(caPattern);
					free(pcInPattern);
					return(-1);
				}
			}
			continue;
		}else{
			if(ret){
				free(caPattern);
				free(pcInPattern);
				return(ret);
			}
			i++;
			cnt=0;
			memset(pcInPattern,0x00,alloc_size);
			iGetPatternUnit(NULL,pcInPattern,caPattern,&offset);
			if(pcInPattern=='\0'){
				free(caPattern);
				free(pcInPattern);
				return(-1);
			}
			FirstMatch=1;
		}
	}
	free(caPattern);
	free(pcInPattern);
	
	if(str[i]!='\0'){		
		return(-1);
	}

	return(ret);
}
/**
函数名:pub_regex_match
功能  :判断字符串与一个模式是否匹配
参数  :
       str       字符串
       pattern   模式   
返回值:
      0          匹配
      -1         不匹配
**/ 
int pub_regex_match(char *str,char *pattern){
	int ret;
	int len;
	if(pattern==NULL || str==NULL){
		return(-1);
	}
	len=strlen(pattern);
	if(pattern[0]=='~'){
		if(pattern[1]!='(' || pattern[len-1]!=')'){
			return(-1);
		}else{
			char *pcNewPattern=(char *)malloc(len+1);
			strcpy(pcNewPattern,pattern);
			pcNewPattern[len-1]='\0';
			ret=nStringMatch_a(str,pcNewPattern+2);
			free(pcNewPattern);
			if(ret==0){
				return(-1);
			}else{
				return(0);
			}
		}
	}else{
		return(nStringMatch_a(str,pattern));
	}	
	return(-1);	
}
/*************
print_match(char *a,char *b){
	int ret;
	ret=nStringMatch(a,b);
	if(ret==0){
		printf("[%s][%s]匹配!\n",a,b);
	}else{
		printf("[%s][%s]不匹配!\n",a,b);
	}
	return(0);
}
int main(){
	print_match("abc","abc");
	print_match("abc","a.c");
	print_match("abc","a.*");
	print_match("abc","a.+");
	print_match("abc","ab?");
	print_match("abc","abb?c");
	print_match("abc","a\\ab?c");
	print_match("abc","a\\ab?cd");
	print_match("abc","a[\\a|b]?c.*");
	print_match("abc","a[0|1|2]+c.*");
	print_match("abc","a[0|1|2]*c.*");
	print_match("abcABC001--*","~(\\a\\a\\a\\A*\\d\\d[1|2|3|4|6|7|8|9]?...)");
	print_match("abc","\\a{3}");
	print_match("abc","\\a{3}.*");
	print_match("abc","[a|b|c]{3}.*");
	print_match("abc","[a|b]{3}.*");
	print_match("abc","[a|b|c]{1,2}c");
	print_match("abc","[a|b|c]{2}.*b");
	print_match("abc","[a|b|c]{4}.*");
	print_match("abc","\\a{3}.+");
	print_match("abc","\\a{3}.?");
	print_match("abc","abc?");
	print_match("abc","abc.?");
	print_match("abcdef","a[b|c|d]{2,11}ef");
	print_match("abcdef","a[b|c|d]{3,11}ef");
	print_match("abcdef","a[b|c|d]{0,11}ef");
}
******************/
