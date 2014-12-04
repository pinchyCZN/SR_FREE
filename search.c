#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <process.h>
#include "resource.h"
#include "trex.h"

extern HWND ghwindow;
static HWND modeless_search_hwnd=0;

int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);

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
int regex_search=FALSE;
int hex_search=FALSE;
unsigned int depth_limit=MAXDWORD;

int files_searched=0;
int files_occured=0;
int matches_found=0;
int total_matches=0;
char current_fname[MAX_PATH]={0};
int match_prefix_len=40;

TRex *trex_regx=0;


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
int is_alphanumeric(unsigned char a)
{
	if((a>='A' && a<='Z')||(a>='a' && a<='z')||(a>='0' && a<='9'))
		return TRUE;
	else
		return FALSE;
}
int is_wordsubset(unsigned char a)
{
	if((a>='A' && a<='Z')||(a>='a' && a<='z')||(a>='0' && a<='9')||(a=='_'))
		return TRUE;
	else
		return FALSE;
}
int convert_char(unsigned char a)
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
int fetch_from_fpos(FILE *f,__int64 fpos,int len,char *buf)
{
	__int64 offset;
	int read;
	offset=_ftelli64(f);
	_fseeki64(f,fpos,SEEK_SET);
	read=fread(buf,1,len,f);
	_fseeki64(f,offset,SEEK_SET);
	return read;
}

int fill_eol(FILE *f,char *str,int str_size,int pos,char *buf,int buf_len,int match_end,
			 int binary,int match_prefix_len)
{
	int len,eol=FALSE;
	len=str_size-pos;
	if(len>0){
		int index=pos;
		int count=0;
		int non_alpha_found=FALSE;
		if(match_end<buf_len){
			int i;
			for(i=0;i<buf_len-match_end;i++){
				unsigned char a=buf[match_end+i];
				if(binary){
					count++;
					if(a<' ')
						non_alpha_found=TRUE;
					if(non_alpha_found && (count>match_prefix_len)){
						eol=TRUE;
						break;
					}
				}
				else if(a=='\n' || a=='\r'){
					eol=TRUE;
					break;
				}
				else if(a<' ')
					binary=TRUE;

				str[index++]=convert_char(a);
				if(index>=str_size)
					break;
			}
			len-=i;
			if(index<str_size)
				str[index]=0;
		}
		if(!eol){
			if(len>0){
				char tmp[512];
				int i,b=0;
				if(len>sizeof(tmp))
					len=sizeof(tmp);

				len=fetch_more_data(f,tmp,len,&b);
				for(i=0;i<len;i++){
					unsigned char a;
					if(index>=str_size)
						break;
					if(binary){
						count++;
						if(a<' ')
							non_alpha_found=TRUE;
						if(non_alpha_found && (count>match_prefix_len)){
							break;
						}
					}
					else if(a=='\n' || a=='\r')
						break;
					a=tmp[i];
					str[index++]=a;

				}
				if(index<str_size)
					str[index]=0;
			}
		}
	}
	return 0;
}
int fill_begin_line(FILE *f,__int64 fpos,char *str,int str_size,char *buf,int pos,int match_size,int match_prefix_len,int *line_col,
					int binary)
{
	int i;
	int index=0;
	if(pos>=match_prefix_len){
		for(i=0;i<match_prefix_len;i++){
			char a=buf[pos-match_prefix_len+i];
			if((!binary) && (a=='\n' || a=='\r'))
				index=0;
			else
				str[index++]=convert_char(a);
			if(index>=str_size)
				break;
		}
		if(index<str_size)
			str[index]=0;
	}else{
		char tmp[512];
		__int64 offset;
		int len;
		len=match_prefix_len-pos;
		if(len>sizeof(tmp))
			len=sizeof(tmp);
		if((fpos-len)<0){
			offset=0;
			len=(int)fpos;
		}
		else
			offset=fpos-len;
		len=fetch_from_fpos(f,offset,len,tmp);
		for(i=0;i<len+pos;i++){
			char a;
			if(i>=len)
				a=buf[i-len];
			else
				a=tmp[i];
			if((!binary) && (a=='\n' || a=='\r'))
				index=0;
			else
				str[index++]=convert_char(a);
			if(index>=str_size)
				break;
		}
		if(index<str_size)
			str[index]=0;
	}
	line_col[0]=index;
	for(i=0;i<match_size;i++){
		char a=buf[pos+i];
		if(index>=str_size)
			break;
		str[index++]=convert_char(a);
	}
	if(index<str_size)
		str[index]=0;
	return index;
}

int search_buffer_regex(FILE *f,HWND hwnd,int init,unsigned char *buf,int len,int eof)
{
	static void (*compile)(char const *, size_t);
	static size_t (*execute)(char const *, size_t, size_t *, int);
	static unsigned __int64 offset=0;
	static unsigned __int64 line_num=1;
	static unsigned __int64 col_pos=1;
	unsigned __int64 cur_offset=0;
	int partial_match=0;
	const TRexChar *begin=0,*end=0;
	if(init){
		offset=0;
		col_pos=line_num=1;
		return TRUE;
	}
	while(trex_searchrange(trex_regx,buf+cur_offset,buf+len,&begin,&end,&line_num,&col_pos,&partial_match)){
		HWND hwnd_parent=ghwindow;
		int lb_index;
		unsigned char str[512];
		int i,pos,match_size,line_col=0;
		unsigned char *s;
		
		match_size=end-begin;
		pos=begin-buf;
		cur_offset=pos+match_size;
		s=(unsigned char*)begin;
		
		if(matches_found==0){
			add_listbox_str(hwnd_parent,"File %s",current_fname);
			files_occured++;
		}
		matches_found++;

		i=fill_begin_line(f,offset,str,sizeof(str),buf,pos,match_size,match_prefix_len,&line_col,0);
		fill_eol(f,str,sizeof(str),i,buf,len,pos+match_size,0,match_prefix_len);
		str[sizeof(str)-1]=0;
		lb_index=add_listbox_str(hwnd_parent,"Line %I64i col %I64i = %i %i %I64X -%s",line_num,col_pos,line_col,match_size,offset+pos,str);
		if(do_replace)
			replace_dlg(hwnd,lb_index);

		partial_match=0;

		for(i=0;i<end-begin;i++){
			if(begin[i]=='\n')
				col_pos=1;
			else
				col_pos++;
		}
		if(stop_thread)
			break;
	}
	if(!eof){
		if(partial_match>0){
			int i;
			if(partial_match>=len)
				partial_match=len-1;
			for(i=0;i<partial_match;i++){
				char a=buf[len+i-1];
				if(a=='\n' || a=='\r')
					col_pos=1;
				else{
					col_pos--;
					if(col_pos<=0){
						col_pos=1;
						break;
					}
				}
			}
			fseek(f,-partial_match,SEEK_CUR);
			offset-=partial_match;
		}
	}
	offset+=len;
	return 0;
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
					if(leading_repeat==0 && match_offset>0) //if target string has repeats
						j--;
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
							else{
								match_offset--;
								j--;
							}
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
					if(start_pos+l>match_prefix_len){
						int x=l;
						if(x>match_prefix_len)
							x=match_prefix_len;
						s=line+start_pos+x-match_prefix_len;
						c=match_prefix_len-x;
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
				start=start_pos-match_prefix_len;
				if(start<=0){
					goto reset;
				}
				else{
					len=sizeof(line)-2-(start_pos-match_prefix_len)+1;
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
int add_line_match(HWND hwnd,__int64 *offset,__int64 *line,__int64 *col,int match_pos,int match_len,char *str)
{
	return add_listbox_str(hwnd,"Line %I64i col %I64i = %i %i %I64X -%s",*line,*col,match_pos,match_len,*offset,str);
}
int add_binary_match(HWND hwnd,__int64 *offset,__int64 *line,int match_pos,int match_len,char *str)
{
	return add_listbox_str(hwnd,"Offset 0x%I64X = %I64i %i %i -%s",*offset,*line,match_pos,match_len,str);
}
int search_buffer(FILE *f,HWND hwnd,int init,char *buf,int len,int eof)
{
	static int binary=0;
	static __int64 offset=0;
	static __int64 line_num=0;
	static __int64 col=0;
	int pos,partial_match=0;

	if(regex_search)
		return search_buffer_regex(f,hwnd,init,buf,len,eof);
	else if(wildcard_search)
		return search_buffer_wildcard(f,hwnd,init,buf,len,eof);
	if(init){
		binary=FALSE;
		offset=0;
		line_num=1;
		col=1;
		return 0;
	}
	pos=0;
	while(pos<len){
		int i;
		int found=TRUE;
		if(stop_thread)
			break;
		for(i=0;i<strlen_search_str;i++){
			char a,b;
			if((pos+i)>=len)
				break;
			a=buf[pos+i];
			b=search_str[i];
			if(a==0 || b==0)
				binary=TRUE;
			if(!case_sensitive){
				a=upper_case(a);
				b=upper_case(b);
			}
			if(a!=b){
				found=FALSE;
				break;
			}
		}
		if(i<strlen_search_str && found){
			partial_match=i;
			break;
		}
		if(found){
			HWND hwnd_parent=ghwindow;
			int match_len=strlen_search_str;
			int line_col=0;
			int lb_index=-1;
			char str[512];
			__int64 c_offset;
			c_offset=offset+pos;
			if(matches_found==0){
				add_listbox_str(hwnd_parent,"File %s",current_fname);
				files_occured++;
			}
			i=fill_begin_line(f,offset,str,sizeof(str),buf,pos,match_len,match_prefix_len,&line_col,binary);
			fill_eol(f,str,sizeof(str),i,buf,len,pos+match_len,binary,match_prefix_len);
			str[sizeof(str)-1]=0;
			if(binary){
				lb_index=add_binary_match(hwnd_parent,&c_offset,&line_num,line_col,match_len,str);
			}
			else
				lb_index=add_line_match(hwnd_parent,&c_offset,&line_num,&col,line_col,match_len,str);
			if(do_replace)
				replace_dlg(hwnd,lb_index);
			matches_found++;
		}
		if(buf[pos]=='\n'){
			line_num++;
			col=1;
		}else{
			col++;
		}
		pos++;
	}
	offset+=len;
	if((!eof) && (partial_match>0)){
		if(partial_match<len){
			fseek(f,-partial_match,SEEK_CUR);
			offset-=partial_match;
		}
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
//		int size=0x100000;
		int size=4;
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
	regex_search=is_button_checked(ghwindow,IDC_REGEX);
	ignore_whitespace=is_button_checked(ghwindow,IDC_IGNOREWS);
	match_whole=is_button_checked(ghwindow,IDC_WHOLEWORD);
	hex_search=is_button_checked(ghwindow,IDC_HEX);
	if(hex_search){
		strlen_search_str=convert_hex_str(search_str,sizeof(search_str));
		strlen_replace_str=convert_hex_str(replace_str,sizeof(replace_str));
	}
	get_ini_value("OPTIONS","match_prefix_len",&match_prefix_len);
	i=1;
	get_ini_value("OPTIONS","show_column",&i);
	set_show_column(i);

	if(wildcard_search){
		if(strchr(search_str,'*')==0 && strchr(search_str,'?')==0)
			wildcard_search=FALSE;
	}

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
			if(!is_path_directory(path)){
				add_listbox_str(ghwindow,"Invalid directory:%s",path);
				continue;
			}
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

LRESULT CALLBACK search_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0,hbutton=0;
	static void  *button_proc=0;
	if(FALSE)
	if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_DRAWITEM
		&&msg!=WM_CTLCOLORBTN&&msg!=WM_CTLCOLOREDIT&&msg!=WM_CTLCOLORSTATIC)
	//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf(">");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}
	if(hwnd==hbutton && button_proc!=0){
		switch(msg){
		case WM_KEYFIRST:
			switch(wparam){
			case VK_ESCAPE:
				SendMessage(hwnd,BM_CLICK,0,0);
				break;
			case VK_PRIOR:
			case VK_NEXT:
			case VK_END:
			case VK_HOME:
			case VK_LEFT:
			case VK_UP:
			case VK_RIGHT:
			case VK_DOWN:
			case VK_TAB:
				{
					HWND h=GetDlgItem(ghwindow,IDC_LIST1);
					if(h)SetFocus(h);
				}
				break;
			}
			break;
		}
		return CallWindowProc(button_proc,hwnd,msg,wparam,lparam);
	}
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			if(!thread_busy){
				stop_thread=FALSE;
				_beginthread(search_thread,0,hwnd);
			}
			else{
				modeless_search_hwnd=0;
				EndDialog(hwnd,0);
			}
			grippy=create_grippy(hwnd);
			SetFocus(GetDlgItem(hwnd,IDCANCEL));
			load_window_pos_relative(GetParent(hwnd),hwnd,"SEARCH_STATUS_WINDOW");
			hbutton=GetDlgItem(hwnd,IDCANCEL);
			button_proc=0;
			if(hbutton)
				button_proc=SetWindowLong(hbutton,GWL_WNDPROC,search_proc);
		}
		return 0;
	case WM_HSCROLL:
		PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),WM_VSCROLL,wparam,lparam);
		break;
	case WM_MOUSEWHEEL:
		SetFocus(GetDlgItem(ghwindow,IDC_LIST1));
		/*
		if(LOWORD(wparam)==MK_RBUTTON){
			WPARAM scrollcode=SB_PAGEUP;
			if(HIWORD(wparam)&0x8000)
				scrollcode=SB_PAGEDOWN;
			PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),WM_VSCROLL,scrollcode,0);
		}
		else
			PostMessage(GetDlgItem(GetParent(hwnd),IDC_LIST1),msg,wparam,lparam);
		*/
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_search(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_APP:
		save_window_pos_relative(ghwindow,hwnd,"SEARCH_STATUS_WINDOW");
		modeless_search_hwnd=0;
		if(total_matches>0){
			HWND h=GetDlgItem(ghwindow,IDC_LIST1);
			if(h)SetFocus(h);
		}
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
	if(is_button_checked(ghwindow,IDC_REGEX)){
		const TRexChar *error = NULL;
		if(trex_regx)
			trex_free(trex_regx);
		trex_regx=trex_compile(search_str,is_button_checked(ghwindow,IDC_CASE),&error);
		if(trex_regx==NULL){
			set_status_bar(hwnd,"Regex failed to compile:%s",error);
			FlashWindow(hwnd,TRUE);
			return 0;
		}
	}

	get_leading_repeats(search_str,strlen_search_str);
	search_str[sizeof(search_str)-1]=0;
	replace_str[sizeof(replace_str)-1]=0;
	save_combo_edit_ctrl(hwnd);
	if(replace)
		DialogBox(ghinstance,MAKEINTRESOURCE(IDD_SEARCH_PROGRESS),hwnd,search_proc);
	else
	{
		if(modeless_search_hwnd==0)
			modeless_search_hwnd=CreateDialog(ghinstance,MAKEINTRESOURCE(IDD_SEARCH_PROGRESS),hwnd,search_proc);
		if(modeless_search_hwnd){
			SetWindowPos(modeless_search_hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
			//ShowWindow(modeless_search_hwnd,SW_SHOW);
		}
		return 0;
	}
	return total_matches>0;
}
int cancel_search()
{
	return stop_thread=TRUE;
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