#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <process.h>
#include "resource.h"

extern HWND ghwindow;
static HWND modeless_search_hwnd=0;

int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);
//static HWND hwnd_parent;

int thread_busy=FALSE;
int stop_thread=FALSE;

char search_str[0x10000];
int strlen_search_str=0;
char replace_str[0x10000];
int strlen_replace_str=0;
int leading_repeat=0;

int replace_rest_file=FALSE;
int replace_all_remain=FALSE;
int skip_rest_file=FALSE;
int cancel_all=FALSE;

int do_replace=FALSE;
int case_sensitive=FALSE;
int match_whole=FALSE;
int ignore_whitespace=FALSE;
int unicode_search=FALSE;
int wildcard_search=FALSE;
int hex_search=FALSE;
unsigned int depth_limit=MAXDWORD;

int files_searched=0;
int files_occured=0;
int matches_found=0;
int total_matches=0;
char current_fname[MAX_PATH]={0};
int hit_line_len=128;

int set_replace_rest_file(int i)
{	return replace_rest_file=i; }
int set_replace_all_remain(int i)
{	return replace_all_remain=i; }
int set_skip_rest_file(int i)
{	return skip_rest_file=i; }

int get_replace_rest_file()
{	return replace_rest_file; }
int get_replace_all_remain()
{	return replace_all_remain; }
int get_skip_rest_file()
{	return skip_rest_file; }

int set_cancel_all(int i)
{	
	if(i)stop_thread=TRUE;
	return cancel_all=i;
}
int get_current_fname(char *fname,int len)
{
	return strncpy(fname,current_fname,len);
}
int get_search_str(char **sstr,int *slen)
{
	if(sstr!=0)
		sstr[0]=search_str;
	if(slen!=0)
		slen[0]=strlen_search_str;
	return TRUE;
}
int get_replace_str(char **rstr,int *rlen)
{
	if(rstr!=0)
		rstr[0]=replace_str;
	if(rlen!=0)
		rlen[0]=strlen_replace_str;
	return TRUE;
}
int upper_case(char a)
{
	if(((unsigned char)a)>='a' && ((unsigned char)a)<='z')
		return a&(' '^0xFF);
	else
		return a;
}
int wild_card_match(const char *match,const char *str)
{
	const char *mp;
	const char *cp = NULL;

	while (*str){
		if (*match == '*'){
			if (!*++match)
				return TRUE;
			mp = match;
			cp = str + 1;
		}
		else if (*match == '?' || upper_case(*match) == upper_case(*str)){
			match++;
			str++;
		}
		else if (!cp)
			return FALSE;
		else{
			match = mp;
			str = cp++;
		}
	}

	while (*match == '*')
		match++;

	return !*match;
}
int str_trim_right(char *s)
{
	int i,len=strlen(s);
	for(i=len-1;i>=0;i--){
		if(s[i]>' ')
			break;
		else
			s[i]=0;
	}
	return TRUE;
}
int does_file_match(char *mask,char *fname,char *path)
{
	char m1[MAX_PATH]={0};
	int i,len,index=0;
	int fmatch=FALSE,pmatch=TRUE;
	len=strlen(mask);
	for(i=0;i<len;i++){
		int exclude=FALSE;
		if(mask[i]==';' || index>=(MAX_PATH-1) || i>=(len-1)){
			char *m;
			if(i>=(len-1) && mask[i]!=';')
				m1[index++]=mask[i];
			m1[index++]=0;
			index=0;
			if(strlen(m1)==0)
				continue;
			m=m1;
			if(m[0]=='~'){
				exclude=TRUE;
				m++;
			}
			if(m[0]=='\\'){
				m++;
				if(wild_card_match(m,path)){
					if(exclude)
						pmatch=FALSE;
					else
						pmatch=TRUE;
				}
				else if(!exclude)
					pmatch=FALSE;
			}
			else if(wild_card_match(m,fname)){
				if(exclude)
					fmatch=FALSE;
				else
					fmatch=TRUE;
			}
		}
		else
			m1[index++]=mask[i];
	}
	return fmatch && pmatch;
}
int set_progress_bar(HWND hwnd,double percent)
{
	SendDlgItemMessage(hwnd,IDC_PROGRESS1,PBM_SETPOS,(int)(percent+0.5),0);
	return TRUE;
}
int add_listbox_str(HWND hwnd,char *fmt,...)
{
	char str[1024]={0};
	va_list args;
	va_start(args,fmt);
	_vsnprintf(str,sizeof(str),fmt,args);
	str[sizeof(str)-1]=0;
	return SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
}
int set_message_str(HWND hwnd,char *fmt,...)
{
	char str[MAX_PATH*2]={0};
	va_list args;
	va_start(args,fmt);
	_vsnprintf(str,sizeof(str),fmt,args);
	str[sizeof(str)-1]=0;
	SetDlgItemText(hwnd,IDC_SEARCH_STATUS,str);
	return TRUE;
}
int convert_hex_str(char *str,int size)
{
	int i,len,index=0;
	char *s=str;
	while(strlen(s)>0){
		int hex,found=FALSE;
		len=strlen(s);
		if(len<=0 || len>=size-1)
			break;
		for(i=0;i<len;i++){
			char a=upper_case(s[i]);
			if(s[i]==0)
				break;
			if((a>='0' && a<='9') || (a>='A' && a<='F')){
				s=s+i;
				found=TRUE;
				break;
			}
		}
		if(!found)
			break;
		hex=strtoul(s,&s,16);
		str[index++]=hex;
		if(index>=size-1)
			break;
		if(s==0)
			break;
	}
	str[index]=0;
	return index;
}
int is_alphanumeric(char a)
{
	if((a>='A' && a<='Z')||(a>='a' && a<='z')||(a>='0' && a<='9'))
		return TRUE;
	else
		return FALSE;
}
int is_wordsubset(char a)
{
	if((a>='A' && a<='Z')||(a>='a' && a<='z')||(a>='0' && a<='9')||(a=='_'))
		return TRUE;
	else
		return FALSE;
}
int convert_char(char a)
{
	if(a=='\t' || a=='\r')
		return ' ';
	else if(a<' '  || a>0x7E)
		return '.';
	else
		return a;
}
int fetch_next_byte(FILE *f,char *a)
{
	char buf[4];
	int read;
	__int64 offset;
	offset=_ftelli64(f);
	read=fread(buf,1,1,f);
	_fseeki64(f,offset,SEEK_SET);
	if(read>0){
		a[0]=buf[0];
		return TRUE;
	}
	else
		return FALSE;
}
int fetch_more_data(FILE *f,char *buf,int len,int *binary)
{
	__int64 offset;
	int i,read;
	offset=_ftelli64(f);
	read=fread(buf,1,len,f);
	for(i=0;i<read;i++){
		if(!binary[0]){
			if(buf[i]==0){
				binary[0]=TRUE;
				break;
			}
			if(buf[i]=='\r'||buf[i]=='\n')
				break;
		}
		buf[i]=convert_char(buf[i]);
	}
	_fseeki64(f,offset,SEEK_SET);
	return i;
}
int search_buffer_wildcard(FILE *f,HWND hwnd,int init,char *buf,int len,int eof)
{
	static char line[512];
	static int line_index=0;
	static __int64 offset=0,line_offset=0;
	static __int64 line_num=1;
	static __int64 total_col=0;
	static int binary=FALSE;
	static int match_offset=0,match_len=0;
	int i;
	if(init){
		memset(line,0,sizeof(line));
		line_num=1;
		line_index=0;
		match_offset=0;
		match_len=strlen_search_str;
		total_col=0;
		line_offset=offset=0;
		binary=FALSE;
		return TRUE;
	}
	for(i=0;i<len;i++){
		if(stop_thread)
			break;
		if(line_index==0)
			line_offset=offset;
		line[line_index]=buf[i];
		if(buf[i]==0)
			binary=TRUE;
		if(buf[i]=='\n' || line_index>=sizeof(line)-2 || (eof && i==len-1)){
			int j;
			int start_pos=-1;
			int col_pos=0;
			int wpos=-1;
			line[sizeof(line)-1]=0;
			if(buf[i]=='\n'){
				line[line_index]=0;
				line_index--;
				if(line_index>=0){
					if(line[line_index]=='\r'){
						line[line_index]=0;
						line_index--;
					}
				}
			}
			total_col+=line_index+1;
			for(j=0;j<line_index+1;j++){
				char a,b;
				if(stop_thread)
					break;
				if(case_sensitive){
					a=search_str[match_offset];
					b=line[j];
				}else{
					a=upper_case(search_str[match_offset]);
					b=upper_case(line[j]);
				}
				if(a=='*'){
					match_offset++;
					if(start_pos<0)
						start_pos=j;
					wpos=start_pos+1;
					if(j>0)
						j--;
				}
				else if(a=='?' || a==b){
					match_offset++;
					if(start_pos<0)
						start_pos=j;
				}
				else if(wpos<0){
					if(leading_repeat>0){ //naive search
						j-=match_offset;
					}
					start_pos=-1;
					match_offset=0;
				}
				else if(wpos>=0){
					if(search_str[match_offset-1]!='*'){
						if(match_offset>0){
							if(leading_repeat>0){ //naive search
								if(j>match_offset && j>wpos){
									int k=match_offset;
									for( ;k>0;k--){
										if(search_str[k]=='*'){
											break;
										}
										else if(search_str[k]=='?'){
											k--;
											break;
										}
									}
									if(k==0)
										k=wpos;
									j-=k;
									match_offset-=k;
								}
							}
							else
								match_offset--;
						}
					}
				}
				else{
					wpos=-1;
					match_offset=0;
				}
				if(match_offset==1)
					col_pos=j;
				if(match_offset>=match_len){
					HWND hwnd_parent=ghwindow;
					char *s=line;
					int c,l,k,lb_index=-1;
					c=start_pos;
					l=j-start_pos+1;
					//if(line_offset+start_pos==0x1461E5B)
					//	l=l;
					for(k=0;k<line_index+1;k++){
						if(line[k]=='\t')
							continue;
						if(line[k]<' '||line[k]>0x7E)
							line[k]='.';
					}
					if(start_pos+l>hit_line_len){
						int x=l;
						if(x>hit_line_len)
							x=hit_line_len;
						s=line+start_pos+x-hit_line_len;
						c=hit_line_len-x;
					}

					if(matches_found==0){
						add_listbox_str(hwnd_parent,"File %s",current_fname);
						files_occured++;
					}
					if(binary){
						line[line_index+1]=0;
						lb_index=add_listbox_str(hwnd_parent,"Offset 0x%I64X = %I64i %i %i -%s",line_offset+start_pos,line_num,c,l,s);
					}
					else
						lb_index=add_listbox_str(hwnd_parent,"Line %I64i col %I64i = %i %i %I64X -%s",
						line_num,total_col-line_index-1+col_pos+1,c,l,line_offset+start_pos,s);
					if(do_replace)
						replace_dlg(hwnd,lb_index);
					match_offset=0;
					matches_found++;
					start_pos=wpos=-1;
					
				}
			}//for(j=0;j<line_index+1;j++){
			if(buf[i]=='\n'){
				line_num++;
				total_col=0;
			}
			if(line_index>=sizeof(line)-2 && match_offset>0 && start_pos>=0){
				int start,len;
				start=start_pos-hit_line_len;
				if(start<=0){
					goto reset;
				}
				else{
					len=sizeof(line)-2-(start_pos-hit_line_len)+1;
					if(len<0)
						len=0;
				}
				memcpy(line,line+start,len);
				total_col-=len;
				line_index=len;
				line_offset+=start;
				match_offset=0;
			}else{
reset:
				match_offset=0;
				line_index=0;
				memset(line,0,sizeof(line));
			}
		}//if(buf[i]=='\n' || line_index>=sizeof(line)-2){
		else{
			line_index++;
			if(line_index>=sizeof(line))
				line_index=0;
		}
		offset++;
	}
	return 0;
}
int search_buffer(FILE *f,HWND hwnd,int init,char *buf,int len,int eof)
{
	int i;
	static char line[512]={0};
	static int line_pos=0;
	static int line_col=0;
	static __int64 offset;
	static __int64 line_num=0;
	static __int64 total_col=0;
	static int match_offset=0;
	static int match_len=0;
	static int found=FALSE;
	static int binary=FALSE;
	static int sizeof_line=sizeof(line);
	static int nibble=0;
	int compare;
	if(wildcard_search)
		return search_buffer_wildcard(f,hwnd,init,buf,len,eof);
	if(init){
		binary=FALSE;
		found=FALSE;
		line_pos=0;
		line_num=1;
		offset=0;
		line_col=1;
		total_col=1;
		match_offset=0;
		match_len=strlen_search_str;
		sizeof_line=hit_line_len;
		nibble=0;
		if(sizeof_line>sizeof(line))
			sizeof_line=sizeof(line);
		memset(line,0,sizeof(line));
		return 0;
	}
	for(i=0;i<len;i++){
		if(stop_thread)
			break;
		if(buf[i]==0)
			binary=TRUE;
		if(match_offset<sizeof_line)
			line[line_pos]=buf[i];
		if(buf[i]=='\n'){
			line_num++;
			total_col=1;
		}
		else
			total_col++;
		if(unicode_search){
			if(nibble==1){
				nibble=0;
				if(buf[i]!=0){
					found=FALSE;
					match_offset=0;
					goto check_nibble;
				}
			}
			else{
check_nibble:
				//if(match_offset==match_len-1)
				//;//	printf("%s\n",line);
				if(case_sensitive)
					compare=(search_str[match_offset]==buf[i]);
				else
					compare=(upper_case(search_str[match_offset])==upper_case(buf[i]));

				if(compare){
					match_offset++;
					nibble=1;
				}
				else if(match_offset>0){
					if(leading_repeat>0){ //naive search
						int delta=match_offset;
						if(delta==0)
							;
						else{
							delta--;
							delta*=2;
						}
						i-=delta;
						line_pos-=delta;
						total_col-=delta;
						offset-=delta;
					}
					if(case_sensitive)
						compare=(search_str[0]==buf[i]);
					else
						compare=(upper_case(search_str[0])==upper_case(buf[i]));

					if(compare){
						line_col=line_pos;
						match_offset=1;
					}
					else{
						found=FALSE;
						if(match_offset>0){
							int delta=match_offset;
							delta*=2;
							i-=delta;
							line_pos-=delta;
							total_col-=delta;
							offset-=delta;
						}

						match_offset=0;
						nibble=0;
					}
				}
			}
		}
		else{
			/*---------NON unicode------------------*/
			if(case_sensitive)
				compare=(search_str[match_offset]==buf[i]);
			else
				compare=(upper_case(search_str[match_offset])==upper_case(buf[i]));

			if(compare)
				match_offset++;
			else if(match_offset>0){
				if(leading_repeat>0){ //naive search
					i-=match_offset-1;
					line_pos-=match_offset-1;
					total_col-=match_offset-1;
					offset-=match_offset-1;
				}
				if(case_sensitive)
					compare=(search_str[0]==buf[i]);
				else
					compare=(upper_case(search_str[0])==upper_case(buf[i]));

				if(compare){
					line_col=line_pos;
					match_offset=1;
				}
				else{
					found=FALSE;
					match_offset=0;
				}
			}
			if(match_whole){
				if(match_offset==1 && offset>0){
					if(is_alphanumeric(buf[i])){
						int j=line_pos-1;
						if(j<0)
							j=sizeof_line-1;
						if(is_wordsubset(line[j])){
							found=FALSE;
							match_offset=0;
						}
					}
				}
				if(match_offset==match_len){
					if(is_alphanumeric(buf[i])){
						if(i+1>=len){
							char a=0;
							//if(FALSE)
							if(fetch_next_byte(f,&a)){
								if(is_wordsubset(a)){
									found=FALSE;
									match_offset=0;
								}
							}
						}
						else if(i+1<len && is_wordsubset(buf[i+1])){
							found=FALSE;
							match_offset=0;
						}
					}
				}
			}
			/*--------------------------------------*/
		}
		if(match_offset>0){
			if(!found){
				line_col=line_pos;
				found=TRUE;
			}
		}
		if(match_offset>=match_len){
			char str[sizeof(line)*2]={0};
			int sizeof_str=sizeof_line*2;
			//if(line_num==480 && total_col-match_len==1)
			//	line_num=line_num;
			//if(offset==0x2503)
			//	offset=offset;
			if((offset>=sizeof_line-1) && line_pos<sizeof_line){
				int j,k,l,m;
				k=0;l=line_pos+1;m=0;
				memset(str,0,sizeof_str);
				//copy previous bytes
				for(j=line_pos+1;j<sizeof_line;j++){
					if(!binary)
					if(line[j]=='\n'){
						k=0;l=j+1;m=0;
						continue;
					}
					str[k++]=convert_char(line[j]);
					m++;
				}
				str[k]=0;
				//copy match part
				for(j=0;j<=line_pos;j++){
					if(!binary)
					if(line[j]=='\n'){
						k=0;l=j+1;
						continue;
					}
					str[k++]=convert_char(line[j]);
				}
				str[k]=0;
				if(k<sizeof_str-1 && match_len<sizeof_line){
					int t;
					int end_found=FALSE;
					//copy rest of line if any
					for(t=1;t<len-i;t++){
						if(!binary){
							if(buf[i+t]==0)
								binary=TRUE;
							if(buf[i+t]=='\r'||buf[i+t]=='\n'){
								end_found=TRUE;
								break;
							}
						}
						str[k++]=convert_char(buf[i+t]);
						if(k>=sizeof_str-1)
							break;
					}
					if(k<sizeof_str-1){
						if(binary)
							k+=fetch_more_data(f,str+k,sizeof_str-k,&binary);
						else if(!end_found)
							k+=fetch_more_data(f,str+k,sizeof_str-k,&binary);
					}
					str[k]=0;
				}
				if(l<=line_col)
					line_col=line_col-l;
				else
					line_col=line_col+m;
				if(line_col<0)
					line_col=0;
			}
			else{
				int k,t,spot=-1,limit;
				k=0;
				if(offset<sizeof_line-1)
					limit=line_pos+1;
				else
					limit=sizeof_line;
				//copy start of line
				for(t=0;t<limit;t++){
					if(!binary){
						if(line[t]==0)
							binary=TRUE;
						else if(line[t]=='\n'){
							if(t<line_col){
								spot=t;
								k=0;
								continue;
							}
							else
								break;
						}
					}
					str[k++]=convert_char(line[t]);
					if(k>=sizeof_str-1)
						break;
				}
				if(spot>=0)
					line_col-=spot+1;
				str[k]=0;
				if(k<sizeof_str-1){
					int end_found=FALSE;
					//fill up rest of string
					for(t=1;t<len-i;t++){
						if(!binary){
							if(buf[i+t]==0)
								binary=TRUE;
							if(buf[i+t]=='\r'||buf[i+t]=='\n'){
								end_found=TRUE;
								break;
							}
						}
						str[k++]=convert_char(buf[i+t]);
						if(k>=sizeof_str-1)
							break;
					}
					if(k<sizeof_str-1){
						if(binary)
							k+=fetch_more_data(f,str+k,sizeof_str-k,&binary);
						else if(!end_found)
							k+=fetch_more_data(f,str+k,sizeof_str-k,&binary);
					}
					str[k]=0;
				}
			}
			{
				HWND hwnd_parent=ghwindow;
				int lb_index=-1;
				if(matches_found==0){
					add_listbox_str(hwnd_parent,"File %s",current_fname);
					files_occured++;
				}
				if(binary){
					if(unicode_search)
						lb_index=add_listbox_str(hwnd_parent,"Offset 0x%I64X = %I64i %i %i -%s",offset-match_len*2+2,line_num,line_col,match_len*2,str);
					else
						lb_index=add_listbox_str(hwnd_parent,"Offset 0x%I64X = %I64i %i %i -%s",offset-match_len+1,line_num,line_col,match_len,str);
					
				}
				else
					lb_index=add_listbox_str(hwnd_parent,"Line %I64i col %I64i = %i %i %I64X -%s",line_num,total_col-match_len,line_col,match_len,offset-match_len+1,str);
				if(do_replace)
					replace_dlg(hwnd,lb_index);
			}
			match_offset=0;
			matches_found++;
			found=FALSE;
		}
		if(match_offset<sizeof_line)
			line_pos++;
		if(line_pos>=sizeof_line)
			line_pos=0;
		offset++;
	}
	return 0;
}
int search_replace_file(HWND hwnd,char *fname,char *path)
{
	FILE *f;
	strncpy(current_fname,path,sizeof(current_fname));
	current_fname[sizeof(current_fname)-1]=0;
	if(current_fname[strlen(current_fname)-1]!='\\')
		_snprintf(current_fname,sizeof(current_fname),"%s\\",current_fname);
	_snprintf(current_fname,sizeof(current_fname),"%s%s",current_fname,fname);
	current_fname[sizeof(current_fname)-1]=0;
	if(strlen_search_str==0){
		add_listbox_str(ghwindow,"File %s",current_fname);
		return FALSE;
	}
	f=fopen(current_fname,"rb");
	if(f!=0){
		char *buf;
		int size=0x100000;
		int read=0;
		buf=malloc(size);
		if(buf!=0){
			unsigned long t1=GetTickCount();
			__int64 flen=0;
			set_message_str(hwnd,"%s",current_fname);
			matches_found=0;
			search_buffer(0,0,TRUE,0,0,0);
//			while((read=fread(buf,1,1+(rand()%12),f))!=0){
			replace_rest_file=FALSE;
			skip_rest_file=FALSE;
			_fseeki64(f,0,SEEK_END);
			flen=_ftelli64(f);
			_fseeki64(f,0,SEEK_SET);
			while((read=fread(buf,1,size,f))!=0){
				__int64 fpos;
				if(stop_thread || skip_rest_file)
					break;
				fpos=_ftelli64(f);
				search_buffer(f,hwnd,FALSE,buf,read,fpos>=flen);
				if((GetTickCount()-t1)>500){
					//set_message_str(hwnd,"%%%.1f",100*(double)tmp/(double)flen);
					set_progress_bar(hwnd,100*(double)fpos/(double)flen);
					t1=GetTickCount();
				}
			}
			total_matches+=matches_found;
			if(matches_found>0)
				add_listbox_str(ghwindow,"found %i matches",matches_found);
			if(flen!=0)
				set_progress_bar(hwnd,0);
			free(buf);
		}
		fclose(f);
		replace_in_file(0,TRUE);
	}
	return TRUE;
}
int combine_path(char *path,char *extra) //assumes path MAX_PATH len
{
	if(path[strlen(path)-1]!='\\')
		_snprintf(path,MAX_PATH,"%s\\",path);
	_snprintf(path,MAX_PATH,"%s%s",path,extra);
	path[MAX_PATH-1]=0;
	return TRUE;
}
int find_files(HWND hwnd,char *path,char *filemask,int *total,
			   int search_sub_dirs,unsigned int max_depth,unsigned int current_depth)
{
	char search_path[MAX_PATH]={0};
	WIN32_FIND_DATA fd;
	HANDLE fhandle;
	if(stop_thread)
		return FALSE;
	if(current_depth>max_depth)
		return FALSE;
	strncpy(search_path,path,sizeof(search_path));
	combine_path(search_path,"*.*");

	fhandle=FindFirstFile(search_path,&fd);
	if(fhandle!=INVALID_HANDLE_VALUE){
		do{
			int result;
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
				result=does_file_match(filemask,fd.cFileName,path);
				if(result){
					search_replace_file(hwnd,fd.cFileName,path);
					files_searched++;
					total[0]++;
				}
			}
			else if(search_sub_dirs){
				if(fd.cFileName[0]!='.'){
					strncpy(search_path,path,sizeof(search_path));
					combine_path(search_path,fd.cFileName);
					find_files(hwnd,search_path,filemask,total,search_sub_dirs,max_depth,current_depth+1);
				}
			}
			if(stop_thread)
				break;
		}while(FindNextFile(fhandle,&fd)!=0);
		FindClose(fhandle);
		return TRUE;
	}
	return FALSE;
}
int search_thread(HWND hwnd)
{
	char filemask[1024]={0};
	char all_paths[1024]={0};
	char path[MAX_PATH]={0};
	WIN32_FIND_DATA fd;
	int i,len,index,total=0;
	int search_sub_dirs=TRUE;
	thread_busy=TRUE;
	memset(&fd,0,sizeof(fd));
	GetDlgItemText(ghwindow,IDC_COMBO_PATH,all_paths,sizeof(all_paths));
	GetDlgItemText(ghwindow,IDC_COMBO_MASK,filemask,sizeof(filemask));

	SendDlgItemMessage(ghwindow,IDC_LIST1,LB_RESETCONTENT,0,0);
	reset_line_width();
	set_status_bar(ghwindow,"");
	case_sensitive=is_button_checked(ghwindow,IDC_CASE);
	unicode_search=is_button_checked(ghwindow,IDC_UNICODE);
	search_sub_dirs=is_button_checked(ghwindow,IDC_SUBDIRS);
	wildcard_search=is_button_checked(ghwindow,IDC_WILDCARD);
	ignore_whitespace=is_button_checked(ghwindow,IDC_IGNOREWS);
	match_whole=is_button_checked(ghwindow,IDC_WHOLEWORD);
	hex_search=is_button_checked(ghwindow,IDC_HEX);
	if(hex_search){
		strlen_search_str=convert_hex_str(search_str,sizeof(search_str));
		strlen_replace_str=convert_hex_str(replace_str,sizeof(replace_str));
	}
	get_ini_value("OPTIONS","hit_width",&hit_line_len);
	i=1;
	get_ini_value("OPTIONS","show_column",&i);
	set_show_column(i);


	replace_all_remain=FALSE;
	files_searched=0;
	files_occured=0;
	total_matches=0;
	len=strlen(all_paths);
	index=0;
	for(i=0;i<len;i++){
		path[index++]=all_paths[i];
		if(path[index-1]==';' || index>=(MAX_PATH-1) || i>=(len-1)){
			if(path[index-1]==';')
				path[index-1]=0;
			path[index++]=0;
			index=0;
			if(strlen(path)==0)
				continue;
			find_files(hwnd,path,filemask,&total,search_sub_dirs,depth_limit,0);
			add_listbox_str(ghwindow,"Searched %i file(s), found %i occurrences in %i file(s)",
				files_searched,total_matches,files_occured);
			if(stop_thread){
				set_status_bar(ghwindow,"search aborted: Searched %i file(s), found %i occurrences in %i file(s)",
					files_searched,total_matches,files_occured);
				break;
			}
			else
				set_status_bar(ghwindow,"Searched %i file(s), found %i occurrences in %i file(s)",
					files_searched,total_matches,files_occured);

		}
	}
	PostMessage(hwnd,WM_APP,0,0);
	thread_busy=FALSE;
	_endthread();
	return 0;
}

LRESULT CALLBACK modal_search_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg)
	{
	case WM_INITDIALOG:
		{
		if(!thread_busy){
			stop_thread=FALSE;
			_beginthread(search_thread,0,hwnd);
		}
		else
			EndDialog(hwnd,0);
		grippy=create_grippy(hwnd);
		SetFocus(GetDlgItem(hwnd,IDCANCEL));
		load_window_pos_relative(GetParent(hwnd),hwnd,"SEARCH_STATUS_WINDOW");
		}
		return 0;
	case WM_HSCROLL:
		PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),WM_VSCROLL,wparam,lparam);
		break;
	case WM_MOUSEWHEEL:
		SetFocus(grippy);
		if(LOWORD(wparam)==MK_RBUTTON){
			WPARAM scrollcode=SB_PAGEUP;
			if(HIWORD(wparam)&0x8000)
				scrollcode=SB_PAGEDOWN;
			PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),WM_VSCROLL,scrollcode,0);
		}
		else
			PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),msg,wparam,lparam);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_search(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_APP:
		save_window_pos_relative(ghwindow,hwnd,"SEARCH_STATUS_WINDOW");
		EndDialog(hwnd,0);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
		case IDCANCEL:
			stop_thread=TRUE;
			return 0;
		}
		break;
	}
	return 0;
}
/* modeless window */
LRESULT CALLBACK search_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	static HWND hbutton=0;
	if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_DRAWITEM
		&&msg!=WM_CTLCOLORBTN&&msg!=WM_CTLCOLOREDIT&&msg!=WM_CTLCOLORSTATIC)
	//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("*");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}

	switch(msg)
	{
	case WM_CREATE:
		grippy=create_grippy(hwnd);
		{
			CREATESTRUCT *cs=lparam;
			hbutton=0;
			if(cs!=0){
				int i;
				HWND h[2]={0,0};
				load_window_pos_relative(cs->hwndParent,hwnd,"SEARCH_STATUS_WINDOW");
				hbutton=h[0]=CreateWindow("BUTTON","Cancel",WS_CHILD|WS_TABSTOP|WS_VISIBLE|BS_PUSHBUTTON,0,0,10,10,hwnd,IDCANCEL,cs->hInstance,0);
				CreateWindow("msctls_progress32","progress",WS_CHILD|WS_VISIBLE|WS_BORDER,0,0,10,10,hwnd,IDC_PROGRESS1,cs->hInstance,0);
				h[1]=CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE,0,0,10,10,hwnd,IDC_SEARCH_STATUS,cs->hInstance,0);
				SetFocus(GetDlgItem(hwnd,IDCANCEL));
				PostMessage(hwnd,WM_SIZE,0,0);
				for(i=0;i<2;i++){
					if(h[i]!=0){
						SendMessage(h[i],WM_SETFONT,GetStockObject(DEFAULT_GUI_FONT),0);
					}
				}
			}
			if(hbutton!=0){
			//	SetWindowLong(hbutton,GWL_WNDPROC	
			}
//			PostMessage(hwnd,WM_APP+1,1,1);
		}
		return 0;
		break;
	case WM_HSCROLL:
		PostMessage(GetDlgItem(ghwindow,IDC_LIST1),WM_VSCROLL,wparam,lparam);
		return 0;
		break;
	case WM_MOUSEWHEEL:
		SetFocus(grippy);
		if(LOWORD(wparam)==MK_RBUTTON){
			WPARAM scrollcode=SB_PAGEUP;
			if(HIWORD(wparam)&0x8000)
				scrollcode=SB_PAGEDOWN;
			PostMessage(GetDlgItem(ghwindow,IDC_LIST1),WM_VSCROLL,scrollcode,0);
		}
		else
			PostMessage(GetDlgItem(ghwindow,IDC_LIST1),msg,wparam,lparam);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_search(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_APP:
		PostMessage(hwnd,WM_CLOSE,0,0);
		break;
	case WM_APP+1:
	case WM_SHOWWINDOW:
		if(wparam){
			if(!thread_busy){
				stop_thread=FALSE;
				_beginthread(search_thread,0,hwnd);
			}
		}
		break;
	case WM_KEYFIRST:
		{
			int key=wparam;
			switch(key){
			case VK_ESCAPE:
				PostMessage(hwnd,WM_CLOSE,0,0);
				break;
			case VK_TAB:
				SetFocus(GetDlgItem(ghwindow,IDC_LIST1));
				break;
			}
		}
		break;
	case WM_QUIT:
	case WM_CLOSE:
		if(thread_busy)
			stop_thread=TRUE;
		save_window_pos_relative(ghwindow,hwnd,"SEARCH_STATUS_WINDOW");
		ShowWindow(hwnd,SW_HIDE);
		return 0;
		//DestroyWindow(hwnd);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			if(HIWORD(wparam)==BN_CLICKED){
				stop_thread=TRUE;
				PostMessage(hwnd,WM_CLOSE,0,0);
			}
			break;
		}
		break;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}
int create_search_win(HWND hwnd,void *sproc,HWND *out)
{
	extern HINSTANCE	ghinstance;
	int result=FALSE;
	static int atom=0;
	const char *class_name="SEARCH_WIN_CLASS";
	if(atom==0){
		WNDCLASS wc;
		memset(&wc,0,sizeof(wc));
		wc.style         = 0;
		if(sproc)
			wc.lpfnWndProc   = sproc;
		else
			wc.lpfnWndProc   = search_proc;
		wc.hInstance     = ghinstance;
		wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
		wc.lpszClassName = class_name;
		atom=RegisterClass(&wc);
	}
	if(atom){
		static HWND hwin=0;
		if(hwin==0){
			hwin=CreateWindow(class_name,"Searching",WS_POPUP|WS_CAPTION|WS_THICKFRAME|WS_SYSMENU,0,0,100,100,hwnd,0,ghinstance,0);
		}
		if(hwin){
			if(out!=0)
				*out=hwnd;
			if(!IsWindowVisible(hwin))
				ShowWindow(hwnd,SW_SHOW);
			result=TRUE;
		}
	}
	return result;
}

int get_leading_repeats(char *str,int len)
{
	int i;
	char a;
	leading_repeat=0;
	if(len>0)
		a=str[0];
	for(i=1;i<len;i++){
		int compare=FALSE;
		if(case_sensitive)
			compare=(a==str[i]);
		else
			compare=(upper_case(a)==upper_case(str[i]));

		if(wildcard_search && ((str[i]=='?') || (str[i]=='*')))
			leading_repeat++;
		else if(compare)
			leading_repeat++;
		else
			break;
	}
	return leading_repeat;
}
int start_search(HWND hwnd,int replace)
{
	extern HINSTANCE	ghinstance;
	char str[sizeof("Binary mode -->>")+1];
#ifdef _DEBUG
	save_ini_stuff(hwnd);
#endif
	do_replace=replace;

	str[0]=0;
	GetDlgItemText(hwnd,IDC_COMBO_SEARCH,str,sizeof(str));
	if(strcmp(str,"Binary mode -->>")!=0){
		search_str[0]=0;
		GetDlgItemText(hwnd,IDC_COMBO_SEARCH,search_str,sizeof(search_str));
		strlen_search_str=strlen(search_str);
	}
	GetDlgItemText(hwnd,IDC_COMBO_REPLACE,str,sizeof(str));
	if(strcmp(str,"Binary mode -->>")!=0){
		replace_str[0]=0;
		GetDlgItemText(hwnd,IDC_COMBO_REPLACE,replace_str,sizeof(replace_str));
		strlen_replace_str=strlen(replace_str);
	}
	get_leading_repeats(search_str,strlen_search_str);
	search_str[sizeof(search_str)-1]=0;
	replace_str[sizeof(replace_str)-1]=0;
	save_combo_edit_ctrl(hwnd);
	if(replace)
		DialogBox(ghinstance,MAKEINTRESOURCE(IDD_SEARCH_PROGRESS),hwnd,modal_search_proc);
	else
	{
		HWND hsearch=0;
		if(create_search_win(hwnd,search_proc,&hsearch)){
			HWND h;
			//h=GetDlgItem(hsearch,IDC_SEARCH_STATUS);
			//SendMessage(hsearch,WM_USER+1,0,0);
			//ShowWindow(hsearch,SW_SHOWNORMAL);
			//PostMessage(hsearch,WM_USER+1,0,0);
			//SendMessage(hsearch,WM_COMMAND,1,1);
		}
		return 0;
	}
	return total_matches>0;
}
int is_thread_busy()
{
	return thread_busy;
}
int set_depth_limit(int limit)
{
	return depth_limit=limit;
}
int get_depth_limit()
{
	return depth_limit;
}