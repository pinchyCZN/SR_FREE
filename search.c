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

int replace_rest_file=FALSE;
int replace_all_remain=FALSE;
int skip_rest_file=FALSE;
int cancel_all=FALSE;

int do_replace=FALSE;
int case_sensitive=FALSE;
int match_whole=FALSE;
int unicode_search=FALSE;
int wildcard_search=FALSE;
int regex_search=FALSE;
int hex_search=FALSE;
unsigned int depth_limit=MAXDWORD;

int files_searched=0;
int files_occured=0;
int matches_found=0;
int total_matches=0;
WCHAR current_fname[SR_MAX_PATH]={0};
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
int get_current_fname(WCHAR *fname,int len)
{
	return wcsncpy(fname,current_fname,len);
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
int upper_case_wc(WCHAR a)
{
	if((a>=L'a') && (a<=L'z'))
		return a&(L' '^0xFF);
	else
		return a;
}
int wild_card_match_wc(const WCHAR *match,const WCHAR *str)
{
	const WCHAR *mp;
	const WCHAR *cp = NULL;

	while (*str){
		if (*match == L'*'){
			if (!*++match)
				return TRUE;
			mp = match;
			cp = str + 1;
		}
		else if (*match == L'?' || upper_case_wc(*match) == upper_case_wc(*str)){
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

	while (*match == L'*')
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
int does_file_match(WCHAR *mask,WCHAR *fname,WCHAR *path)
{
	WCHAR m1[SR_MAX_PATH]={0};
	int i,len,index=0;
	int fmatch=FALSE,pmatch=TRUE;
	len=wcslen(mask);
	for(i=0;i<len;i++){
		int exclude=FALSE;
		if(mask[i]==L';' || index>=(sizeof(m1)/sizeof(WCHAR)-1) || i>=(len-1)){
			WCHAR *m;
			if(i>=(len-1) && mask[i]!=L';')
				m1[index++]=mask[i];
			m1[index++]=0;
			index=0;
			if(wcslen(m1)==0)
				continue;
			m=m1;
			if(m[0]==L'~'){
				exclude=TRUE;
				m++;
			}
			if(m[0]==L'\\'){
				m++;
				if(wild_card_match_wc(m,path)){
					if(exclude)
						pmatch=FALSE;
					else
						pmatch=TRUE;
				}
				else if(!exclude)
					pmatch=FALSE;
			}
			else if(wild_card_match_wc(m,fname)){
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
int is_unc_path(WCHAR *str)
{
	if(str[0]==L'\\' && str[1]==L'\\')
		return TRUE;
	else
		return FALSE;
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
int add_listbox_str_wc(HWND hwnd,WCHAR *fmt,...)
{
	WCHAR str[1024]={0};
	va_list args;
	va_start(args,fmt);
	_vsnwprintf(str,sizeof(str)/sizeof(WCHAR),fmt,args);
	str[sizeof(str)/sizeof(WCHAR)-1]=0;
	return SendDlgItemMessageW(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
}
int set_message_str_wc(HWND hwnd,WCHAR *fmt,...)
{
	WCHAR str[MAX_PATH*2]={0};
	WCHAR *p;
	int set_status2=FALSE;
	va_list args;
	va_start(args,fmt);
	_vsnwprintf(str,sizeof(str)/sizeof(WCHAR),fmt,args);
	str[sizeof(str)/sizeof(WCHAR)-1]=0;
	SetDlgItemTextW(hwnd,IDC_SEARCH_STATUS,str);
	p=wcsrchr(str,L'\\');
	if(p!=0){
		p=wcschr(p+1,L'.');
		if(p!=0){
			SetDlgItemTextW(hwnd,IDC_SEARCH_STATUS2,p);
			set_status2=TRUE;
		}
	}
	if(!set_status2){
		SetDlgItemTextW(hwnd,IDC_SEARCH_STATUS2,L"");
	}
	return TRUE;
}
int is_hex_digit(char a)
{
	int result=FALSE;
	unsigned char b=upper_case(a);
	if((b>='0' && b<='9') || (b>='A' && b<='F'))
		result=TRUE;
	return result;
}
int convert_hex_str(char *str,int size)
{
	int i,index;
	char *s=str;
	i=index=0;
	while(1){
		char a,b;
		a=upper_case(s[i++]);
		b=0;
		if(a=='0'){
			b=s[i];
			if(b=='x'){
				i++;
				a=s[i++];
				b=0;
			}
		}
		if((a>0) && (!is_hex_digit(a))){
			continue;
		}else if(a>0){
			b=s[i++];
		}
		if(a==0)
			break;
		{
			char tmp[3]={0};
			char *ptr=0;
			tmp[0]=a;
			if(is_hex_digit(b))
				tmp[1]=b;
			if(index>=size){
				break;
			}
			str[index++]=(char)strtoul(tmp,&ptr,16);
		}
		
	}
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
	if(a=='\t')
		return a;
	if(a=='\r')
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
				else if(a<' '){
					if(a!='\t')
						binary=TRUE;
				}

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
int add_line_match(HWND hwnd,__int64 *offset,__int64 *line,__int64 *col,int match_pos,int match_len,char *str)
{
	return add_listbox_str(hwnd,"Line %I64i col %I64i = %i %i %I64X -%s",*line,*col,match_pos,match_len,*offset,str);
}
int add_binary_match(HWND hwnd,__int64 *offset,__int64 *line,int match_pos,int match_len,char *str)
{
	return add_listbox_str(hwnd,"Offset 0x%I64X = %I64i %i %i -%s",*offset,*line,match_pos,match_len,str);
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
			add_listbox_str_wc(hwnd_parent,L"File %s",current_fname);
			files_occured++;
		}
		matches_found++;

		i=fill_begin_line(f,offset,str,sizeof(str),buf,pos,match_size,match_prefix_len,&line_col,0);
		fill_eol(f,str,sizeof(str),i,buf,len,pos+match_size,0,match_prefix_len);
		str[sizeof(str)-1]=0;
		{
			__int64 tmp=offset+pos;
			lb_index=add_line_match(hwnd_parent,&tmp,&line_num,&col_pos,line_col,match_size,str);
		}
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
	static int binary=0;
	static __int64 offset=0;
	static __int64 line_num=0;
	static __int64 col=0;
	int pos,partial_match=0;

	if(init){
		binary=FALSE;
		offset=0;
		line_num=1;
		col=1;
		return 0;
	}
	pos=0;
	while(pos<len){
		int found=FALSE;
		int match_len=0;
		if(stop_thread)
			break;
		{
			const char *str,*match;
			const char *mp;
			const char *cp = NULL;
			str=buf+pos;
			match=search_str;
			partial_match=0;
			while (*str){
				char a,b;
				a=*match;
				b=*str;
				if(stop_thread)
					goto EXIT;
				if(!case_sensitive){
					a=upper_case(a);
					b=upper_case(b);
				}
				if (*match == '*'){
					if (!*++match){
						found=TRUE;
						goto EXIT;
					}
					mp = match;
					cp = str + 1;
				}
				else if (*match == '?' || a == b){
					match++;
					str++;
				}
				else if (!cp)
					goto EXIT;
				else{
					if(*match==0){
						found=TRUE;
						goto EXIT;
					}
					match = mp;
					str = cp++;
				}
				if(*match==0){
					break;
				}
				if(str>=(buf+len)){
					partial_match=str-(buf+pos);
					goto EXIT2;
				}
				if(*str=='\n')
					break;
				if(str>=(buf+pos+512)){
					break;
				}
			}
			while (*match == '*')
				match++;
			found=!*match;
			if(!found){
				if(match_len>(len-pos)){
					partial_match=len-pos;
					goto EXIT2;
				}
			}
EXIT:
		match_len=str-(buf+pos);
		}
		if(found){
			HWND hwnd_parent=ghwindow;
			int i,line_col=0;
			int lb_index=-1;
			char str[512];
			__int64 c_offset;
			c_offset=offset+pos;
			if(matches_found==0){
				add_listbox_str_wc(hwnd_parent,L"File %s",current_fname);
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
EXIT2:
	offset+=len;
	if((!eof) && (partial_match>0)){
		if(partial_match>=len)
			partial_match=len-1;
		fseek(f,-partial_match,SEEK_CUR);
		offset-=partial_match;
	}
	return 0;
}
int _istoken(unsigned char a)
{
	if((a>='0' && a<='9') || (a>='a' && a<='z') || (a>='A' && a<='Z') || a=='_')
		return TRUE;
	else
		return FALSE;
}
int verify_whole_word(char *str,int pos,int match_len,int str_len)
{
	int a,b;
	if(pos>0){
		a=_istoken(str[pos-1]);
		b=_istoken(str[pos]);
		if(a==b){
			return FALSE;
		}
	}
	if((pos+match_len)<str_len){
		int c;
		a=_istoken(str[pos+match_len-1]);
		b=_istoken(str[pos+match_len]);
		c=str[pos+match_len];
		if(a==b && c!=0){
			return FALSE;
		}
	}
	return TRUE;
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
			if(unicode_search){
				if((pos+i*2+1)>=len)
					break;
				a=buf[pos+i*2];
				if(buf[pos+i*2+1]!=0){
					found=FALSE;
					break;
				}
			}else{
				if((pos+i)>=len)
					break;
				a=buf[pos+i];
			}
			b=search_str[i];
			if(a==0 || b==0)
				binary=TRUE;
			if(!(case_sensitive || hex_search)){
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
			if(unicode_search)
				match_len*=2;
			i=fill_begin_line(f,offset,str,sizeof(str),buf,pos,match_len,match_prefix_len,&line_col,binary);
			fill_eol(f,str,sizeof(str),i,buf,len,pos+match_len,binary,match_prefix_len);
			str[sizeof(str)-1]=0;
			if(match_whole)
				found=verify_whole_word(str,line_col,match_len,sizeof(str));
			if(found){
				if(matches_found==0){
					add_listbox_str_wc(hwnd_parent,L"File %s",current_fname);
					files_occured++;
				}
				if(binary){
					lb_index=add_binary_match(hwnd_parent,&c_offset,&line_num,line_col,match_len,str);
				}
				else
					lb_index=add_line_match(hwnd_parent,&c_offset,&line_num,&col,line_col,match_len,str);
				if(do_replace)
					replace_dlg(hwnd,lb_index);
				matches_found++;
			}
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
int search_replace_file(HWND hwnd,WCHAR *fname,WCHAR *path)
{
	FILE *f;
	static WCHAR full_path[SR_MAX_PATH];
	if(is_unc_path(path))
		_snwprintf(full_path,sizeof(full_path)/sizeof(WCHAR),L"\\\\?\\UNC%s\\%s",path+1,fname);
	else
		_snwprintf(full_path,sizeof(full_path)/sizeof(WCHAR),L"\\\\?\\%s\\%s",path,fname);
	full_path[sizeof(full_path)/sizeof(WCHAR)-1]=0;

	_snwprintf(current_fname,sizeof(current_fname)/sizeof(WCHAR),L"%s\\%s",path,fname);
	current_fname[sizeof(current_fname)/sizeof(WCHAR)-1]=0;

	if(strlen_search_str==0){
		add_listbox_str_wc(ghwindow,L"File %s",current_fname);
		return FALSE;
	}
	f=_wfopen(full_path,L"rb");
#ifdef _DEBUG
	#define _TEST 1
#endif
	if(f!=0){
		char *buf;
#if _TEST
		int size=0x100000;
#else
		int size=0x100000;
#endif
		int read=0;
#if _TEST
		buf=malloc(size+1);
#else
		buf=malloc(size);
#endif
		if(buf!=0){
			unsigned long t1=GetTickCount();
			__int64 flen=0;
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
#if _TEST
buf[read]=0;
#endif
				fpos=_ftelli64(f);
				search_buffer(f,hwnd,FALSE,buf,read,fpos>=flen);
				if((GetTickCount()-t1)>500){
					set_message_str_wc(hwnd,L"%s",current_fname);
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
		replace_in_file(0,0,TRUE);
	}
	return TRUE;
}
int find_files(HWND hwnd,WCHAR *path,WCHAR *filemask,int *total,
			   int search_sub_dirs,unsigned int max_depth,unsigned int current_depth)
{
	int result=FALSE;
	WCHAR *search_path=0;
	int search_path_size=SR_MAX_PATH;
	WIN32_FIND_DATAW fd;
	HANDLE fhandle;
	if(stop_thread)
		return result;
	if(current_depth>max_depth)
		return result;
	search_path=malloc(search_path_size*sizeof(WCHAR));
	if(search_path==0)
		return result;
	if(is_unc_path(path))
		_snwprintf(search_path,search_path_size,L"\\\\?\\UNC%s\\*.*",path+1);
	else
		_snwprintf(search_path,search_path_size,L"\\\\?\\%s\\*.*",path);
	search_path[search_path_size-1]=0;
	fhandle=FindFirstFileW(search_path,&fd);
	if(fhandle!=INVALID_HANDLE_VALUE){
		DWORD tick=0,delta;
		do{
			int result;
			if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)){
				result=does_file_match(filemask,fd.cFileName,path);
				if(result){
					delta=GetTickCount();
					if((delta-tick)>150){
						set_message_str_wc(hwnd,L"%s\\%s",path,fd.cFileName);
						tick=delta;
					}
					search_replace_file(hwnd,fd.cFileName,path);
					files_searched++;
					total[0]++;
				}
			}
			else if(search_sub_dirs){
				if((0==wcscmp(fd.cFileName,L"."))
					|| (0==wcscmp(fd.cFileName,L".."))){
					;
				}else{
					_snwprintf(search_path,search_path_size,L"%s\\%s",path,fd.cFileName);
					search_path[search_path_size-1]=0;
					find_files(hwnd,search_path,filemask,total,search_sub_dirs,max_depth,current_depth+1);
				}
			}
			if(stop_thread)
				break;
		}while(FindNextFileW(fhandle,&fd)!=0);
		FindClose(fhandle);
		result=TRUE;
	}
	if(search_path!=0)
		free(search_path);
	return result;
}
int normalize_path(WCHAR *path,int path_size)
{
	int len=wcslen(path);
	if(len>0 && len<=path_size)
		if(path[len-1]==L'\\')
			path[len-1]=0;
	return 0;
}
int search_thread(HWND hwnd)
{
	static WCHAR filemask[MAX_PATH]={0};
	static WCHAR all_paths[SR_MAX_PATH]={0};
	static WCHAR path[SR_MAX_PATH]={0};
	WIN32_FIND_DATAW fd;
	int i,len,index,total=0;
	int search_sub_dirs=TRUE;
	char *orig_search_str=0;
	char *orig_replace_str=0;
	thread_busy=TRUE;
	memset(&fd,0,sizeof(fd));
	GetDlgItemTextW(ghwindow,IDC_COMBO_PATH,all_paths,sizeof(all_paths)/sizeof(WCHAR));
	GetDlgItemTextW(ghwindow,IDC_COMBO_MASK,filemask,sizeof(filemask)/sizeof(WCHAR));

	SendDlgItemMessage(ghwindow,IDC_LIST1,LB_RESETCONTENT,0,0);
	reset_line_width();
	set_status_bar(ghwindow,"");
	case_sensitive=is_button_checked(ghwindow,IDC_CASE);
	unicode_search=is_button_checked(ghwindow,IDC_UNICODE);
	search_sub_dirs=is_button_checked(ghwindow,IDC_SUBDIRS);
	wildcard_search=is_button_checked(ghwindow,IDC_WILDCARD);
	regex_search=is_button_checked(ghwindow,IDC_REGEX);
	match_whole=is_button_checked(ghwindow,IDC_WHOLEWORD);
	hex_search=is_button_checked(ghwindow,IDC_HEX);
	if(hex_search){
		orig_search_str=malloc(sizeof(search_str));
		orig_replace_str=malloc(sizeof(replace_str));
		if(orig_search_str)
			memcpy(orig_search_str,search_str,sizeof(search_str));
		if(orig_replace_str)
			memcpy(orig_replace_str,replace_str,sizeof(replace_str));
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
	len=wcslen(all_paths);
	index=0;
	for(i=0;i<len;i++){
		path[index++]=all_paths[i];
		if(path[index-1]==';' || index>=(sizeof(path)-1) || i>=(len-1)){
			if(path[index-1]==';')
				path[index-1]=0;
			path[index++]=0;
			index=0;
			if(wcslen(path)==0)
				continue;
			if(!is_path_directory_wc(path)){
				add_listbox_str_wc(ghwindow,L"Invalid directory:%s",path);
				continue;
			}
			normalize_path(path,sizeof(path)/sizeof(WCHAR));
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
	if(hex_search){
		if(orig_search_str){
			memcpy(search_str,orig_search_str,sizeof(search_str));
			free(orig_search_str);
		}
		if(orig_replace_str){
			memcpy(replace_str,orig_replace_str,sizeof(replace_str));
			free(orig_replace_str);
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
			stop_thread=FALSE;
			_beginthread(search_thread,0,hwnd);

			grippy=create_grippy(hwnd);
			init_search_prog_win_anchor(hwnd);
			restore_search_prog_rel_pos(hwnd);
			SetFocus(GetDlgItem(hwnd,IDCANCEL));
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
		resize_search_prog(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_APP:
		save_search_prog_rel_pos(hwnd);
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

int start_search(HWND hwnd,int replace)
{
	extern HINSTANCE	ghinstance;
	char str[sizeof("Binary mode -->>")+1];
#ifdef _DEBUG
	save_ini_stuff(hwnd);
#endif
	do_replace=FALSE;

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

	search_str[sizeof(search_str)-1]=0;
	replace_str[sizeof(replace_str)-1]=0;
	save_combo_edit_ctrl(hwnd);
	if(replace){
		if(thread_busy){
			stop_thread=TRUE;
			return 0;
		}
		do_replace=replace;
		DialogBox(ghinstance,MAKEINTRESOURCE(IDD_SEARCH_PROGRESS),hwnd,search_proc);
	}
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