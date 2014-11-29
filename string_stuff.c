#include <windows.h>
#include <stdio.h>
int strstri(char *s1, char *s2) 
{ 
	int i,j,k; 
	for(i=0;s1[i];i++) 
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++) 
			if(!s2[k+1]) 
				return (s1+i); 
	return NULL; 
}

int str_replace(char *str,char *find,char *replace,char **out)
{
	int len;
	char *s;
	s=strstri(str,find);
	if(s!=0){
		int a,b,delta=0;
		char *tmpstr=0;
		len=strlen(str);
		a=strlen(find);
		b=strlen(replace);
		if(b>a)
			delta=b-a;
		tmpstr=malloc(len+delta+1);
		if(tmpstr!=0){
			s[0]=0;
			tmpstr[0]=0;
			_snprintf(tmpstr,len+delta+1,"%s%s%s",str,replace,s+a);
			tmpstr[len+delta]=0;
			out[0]=tmpstr;
			return TRUE;
		}
	}
	return FALSE;
}
int quote_to_pipe_char(char *str)
{
	int i,len;
	len=strlen(str);
	for(i=0;i<len;i++){
		if(str[i]=='\"')
			str[i]='|';
	}
	return TRUE;
}
int pipe_to_quote(char *str)
{
	int i,len;
	len=strlen(str);
	for(i=0;i<len;i++){
		if(str[i]=='|')
			str[i]='\"';
	}
	return TRUE;
}
int shell_execute(HWND hwnd,char *exestr,char *path)
{
	int i,len,pos=0;
	char exe[MAX_PATH]={0};
	if(exestr[0]!='\"')
		return FALSE;
	len=strlen(exestr);
	for(i=1;i<len;i++){
		if(exestr[i]=='\"'){
			pos=i;
			break;
		}
	}
	if(pos==0 || pos>=sizeof(exe))
		return FALSE;
	strncpy(exe,exestr+1,pos);
	exe[pos-1]=0;
	ShellExecute(hwnd,"open",exe,exestr+pos+1,path,SW_SHOWNORMAL);
	return TRUE;
}
int path_up_level(HWND hwnd,int ctrl)
{
	int i,len;
	char str[1024]={0};
	GetDlgItemText(hwnd,ctrl,str,sizeof(str));
	len=strlen(str);
	if(len>1 && str[len-1]=='\\')
		str[len-1]=0;
	if(strchr(str,'\\')==0)
		return FALSE;
	for(i=len-1;i>0;i--){
		if(str[i]=='\\')
			break;
		else
			str[i]=0;
	}
	SetDlgItemText(hwnd,ctrl,str);
	return TRUE;
}