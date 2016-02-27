#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <process.h>
#include <float.h>
#include <math.h>
#include "resource.h"
extern HINSTANCE	ghinstance;
HWND main_hwnd=0;

int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);

char lb_str[1024]={0};
char replaced_str[1024]={0};

int create_replace_str(char *istr,char *ostr,int olen)
{
	int col=0,len=0;
	int is_line=FALSE,result=FALSE;
	__int64 line=0,column=0,offset=0;
	char *rstr=0;
	int rlen=0;
	get_replace_str(&rstr,&rlen);
	if(rstr!=0){
		char *s=strstr(istr,"-");
		if(strnicmp(istr,"line ",sizeof("line ")-1)==0){
			sscanf(istr,"%*s %I64i %*s %I64i %*s %i %i %I64X",&line,&column,&col,&len,&offset);
			is_line=TRUE;
		}
		if(strnicmp(istr,"offset ",sizeof("offset ")-1)==0){
			sscanf(istr,"%*s 0x%I64X %*s %*s %i %i",&offset,&col,&len);
			is_line=FALSE;
		}
		if(len>0 && s!=0){
			char *info,*str;
			int size=2048;
			str=malloc(size);
			info=malloc(size);
			if(str!=0 && info!=0){
				int i,index,replace_index;
				if(is_line)
					_snprintf(info,size,"Line %I64i col %I64i = %i %i %I64X -",line,column,col,rlen,offset);
				else
					_snprintf(info,size,"Offset 0x%I64X = %I64i %i %i -",offset,line,col,rlen);
				index=replace_index=0;
				s=s+1;
				for(i=0;i<size;i++){
					if(i==col)
						index+=len;
					if(i>=col && i<=col+rlen && replace_index<rlen)
						str[i]=rstr[replace_index++];
					else
						str[i]=s[index++];
				}
				_snprintf(ostr,olen,"%s%s",info,str);
				result=TRUE;
			}
			if(str!=0)free(str);
			if(info!=0)free(info);

		}
	}
	return result;
}
int create_new_replace_str(char *str,int size,__int64 offset)
{
	char tmp[1024],*s=0;
	int col=0,len=0;
	__int64 l=0,c=0;
	if(strnicmp(str,"offset",sizeof("offset")-1)==0){
		sscanf(str,"%*s %*s %*s %*s %i %i",&col,&len);
		s=strstr(str,"-");
		if(s!=0){
			s++;
			strncpy(tmp,s,sizeof(tmp));
			tmp[sizeof(tmp)-1]=0;
			_snprintf(str,size,"Offset 0x%I64X = %I64i %i %i -%s",offset,l,col,len,tmp);
		}
	}
	else if(strnicmp(str,"line",sizeof("line")-1)==0){
		sscanf(str,"%*s %I64i %*s %I64i %*s %i %i",&l,&c,&col,&len);
		s=strstr(str,"-");
		if(s!=0){
			s++;
			strncpy(tmp,s,sizeof(tmp));
			tmp[sizeof(tmp)-1]=0;
			_snprintf(str,size,"Line %I64i col %I64i = %i %i %I64X -%s",l,c,col,len,offset,tmp);
		}
	}
	return TRUE;
}
int splitpath_long(WCHAR *fullpath,WCHAR *path,int path_size,WCHAR *fname,int fname_size)
{
	int fpath_len;
	int i,last_slash=0;
	WCHAR fmt[12];
	fpath_len=wcslen(fullpath);
	for(i=fpath_len-1;i>=0;i--){
		WCHAR a=fullpath[i];
		if(a==L'\\'){
			last_slash=i;
			break;
		}
	}
	if(path!=0){
		_snwprintf(fmt,sizeof(fmt)/sizeof(WCHAR),L"%%.%is",last_slash);
		fmt[sizeof(fmt)/sizeof(WCHAR)-1]=0;
		_snwprintf(path,path_size,fmt,fullpath);
		if(path_size>0)
			path[path_size-1]=0;
	}
	if(fname!=0){
		_snwprintf(fname,fname_size,L"%s",fullpath+last_slash+1);
		if(fname_size>0)
			fname[fname_size-1]=0;
	}
	return TRUE;
}
int create_tmp_fname(WCHAR *path,WCHAR *tmp,int len)
{
	int result=FALSE;
	WCHAR *fpath,*fname;
	int size=len;
	int num=0;
	FILE *f=-1;
	fpath=malloc(size*sizeof(WCHAR));
	fname=malloc(size*sizeof(WCHAR));
	if(fpath!=0 && fname!=0){
		splitpath_long(path,fpath,size,fname,size);
		do{
			_snwprintf(tmp,len,L"\\\\?\\%s\\%010i%s",fpath,num,fname);
			wprintf(L"%s\n",tmp);
			if(len>0)
				tmp[len-1]=0;
			if(num>100000){
				tmp[0]=0;
				break;
			}
			f=_wfopen(tmp,L"rb");
			if(f!=0)
				fclose(f);
			num++;
		}while(f!=0);
		if(f==0){
			wprintf(L"tmpfname=%s\n",tmp);
			result=TRUE;
		}
	}
	if(fpath!=0)
		free(fpath);
	if(fname!=0)
		free(fname);
	return result;
}
int move_file(WCHAR *src,WCHAR *dest)
{
	return MoveFileExW(src,dest,MOVEFILE_REPLACE_EXISTING);
}
int move_file_data(FILE *fin,FILE *fout,__int64 len,__int64 *moved)
{
	int size=0x10000;
	char *buf;
	buf=malloc(size);
	if(buf!=0){
		__int64 total=0;
		do{
			int amount,read;
			if(total+size<len)
				amount=size;
			else
				amount=len-total;
			read=fread(buf,1,amount,fin);
			fwrite(buf,1,read,fout);
			total+=read;
			if(read==0)
				break;
		}while(total<len);
		if(total>=len){
			moved[0]=total;
			return TRUE;
		}
	}
	return FALSE;
}
int replace_in_file(HWND hwnd,char *info,int close_file)
{
	static FILE *fin=0,*fout=0;
	static WCHAR fname[SR_MAX_PATH]={0};
	static WCHAR tmp[SR_MAX_PATH]={0};
	static __int64 offset,write_offset,read_offset;
	int is_line=FALSE,match_len=0;
	static int state=0;
	if(close_file){
		if(fout!=0){
			__int64 flen=0;
			if(fin!=0){
				_fseeki64(fin,0,SEEK_END);
				flen=_ftelli64(fin);
				_fseeki64(fin,read_offset,SEEK_SET);
				if(read_offset<flen)
					move_file_data(fin,fout,flen-read_offset,&offset);
			}
			fclose(fout);
		}
		if(fin!=0)
			fclose(fin);
		if(fin!=0 && fout!=0){
			if(!move_file(tmp,fname)){
				WCHAR err[MAX_PATH*2+80];
				_snwprintf(err,sizeof(err)/sizeof(WCHAR),L"failed to move:\r\n%s\r\nto\r\n%s",tmp,fname);
				if(MessageBoxW(hwnd,err,L"File move failed",MB_OKCANCEL|MB_SYSTEMMODAL)==IDCANCEL)
					set_replace_all_remain(FALSE);
			}
		}
		fin=0;
		fout=0;
		state=0;
		fname[0]=0;
		tmp[0]=0;
		return TRUE;
	}
	if(info==0)
		return FALSE;
	switch(state){
	case 0:
		get_current_fname(fname,sizeof(fname)/sizeof(WCHAR));
		if(create_tmp_fname(fname,tmp,sizeof(tmp)/sizeof(WCHAR))){
			WCHAR *lfname;
			int lfname_size=SR_MAX_PATH;
			lfname=malloc(lfname_size*sizeof(WCHAR));
			if(lfname!=0){
				_snwprintf(lfname,lfname_size,L"\\\\?\\%s",fname);
				lfname[lfname_size-1]=0;
				wcsncpy(fname,lfname,sizeof(fname)/sizeof(WCHAR));
				fname[sizeof(fname)/sizeof(WCHAR)-1]=0;
				fout=_wfopen(tmp,L"wb");
				fin=_wfopen(lfname,L"rb");
				wprintf(L"%s\n",lfname);
				free(lfname);
			}
			read_offset=write_offset=0;
		}
		state=1;
		break;
	default:
	case 1:
		break;
	}
	offset=-1;
	if(strnicmp(info,"line ",sizeof("line ")-1)==0){
		sscanf(info,"%*s %*s %*s %*s %*s %*s %i %I64X",&match_len,&offset);
		is_line=TRUE;
	}
	if(strnicmp(info,"offset ",sizeof("offset ")-1)==0){
				//"Offset 0x%I64X = %I64i %i %i -%s"
		sscanf(info,"%*s 0x%I64X %*s %*s %*s %i",&offset,&match_len);
		is_line=FALSE;
	}
	if(fin!=0 && fout!=0 && offset>=0){
		__int64 moved=0;
		if(read_offset<offset)
			move_file_data(fin,fout,offset-read_offset,&moved);
		else if(read_offset>offset){
			_fseeki64(fin,offset,SEEK_SET);
			read_offset=offset;
		}
		read_offset+=moved;
		//if(read_offset==offset)
		if(TRUE)
		{
			char *rstr=0;
			int rlen=0;
			write_offset+=moved;
			get_replace_str(&rstr,&rlen);
			if(rlen>0 && rstr!=0){
				fwrite(rstr,1,rlen,fout);
				write_offset+=rlen;
			}
			_fseeki64(fin,offset+match_len,SEEK_SET);
			read_offset=_ftelli64(fin);
			add_listbox_str(main_hwnd,"Replaced with");
			create_new_replace_str(replaced_str,sizeof(replaced_str),write_offset-rlen);
			add_listbox_str(main_hwnd,"%s",replaced_str);
			add_listbox_str(main_hwnd," ");
			return TRUE;
		}
		else{
			WCHAR err[MAX_PATH+80];
			_snwprintf(err,sizeof(err)/sizeof(WCHAR),L"file data error:\r\n%s",tmp);
			err[sizeof(err)/sizeof(WCHAR)-1]=0;
			if(MessageBoxW(hwnd,err,L"read_offset!=offset",MB_OKCANCEL|MB_SYSTEMMODAL)==IDCANCEL){
				set_replace_all_remain(FALSE);
				set_cancel_all(TRUE);
			}
		}

	}
	return FALSE;

}
LRESULT CALLBACK replace_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			WCHAR fname[MAX_PATH*2]={0};
			char *rstr=0,*sstr=0;
			char str[80*3]={0};
			get_replace_str(&rstr,0);
			get_search_str(&sstr,0);
			_snprintf(str,sizeof(str),"Replace [%.80s] with [%.80s]",sstr,rstr);
			SetWindowText(hwnd,str);
			get_current_fname(fname,sizeof(fname)/sizeof(WCHAR));
			grippy=create_grippy(hwnd);
			SetFocus(GetDlgItem(hwnd,IDC_REPLACE_THIS));
			add_listbox_str_wc(hwnd,"File %s",fname);
			add_listbox_str(hwnd,"%s",lb_str);
			add_listbox_str(hwnd,"Replace with");
			add_listbox_str(hwnd,"%s",replaced_str);
		}
		return 0;
	case WM_SIZE:
		resize_replace(hwnd);
		grippy_move(hwnd,grippy);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_DRAWITEM:
		list_drawitem(hwnd,wparam,lparam);
		set_scroll_width(hwnd,IDC_LIST1);
		return TRUE;
		break;
	case WM_HELP:
		return TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_SKIPTHIS:
			EndDialog(hwnd,0);
			break;
		case IDC_SKIP_REST_FILE:
			set_skip_rest_file(TRUE);
			EndDialog(hwnd,0);
			break;
		case IDC_REPLACE_THIS:
			replace_in_file(hwnd,lb_str,FALSE);
			EndDialog(hwnd,0);
			break;
		case IDC_REPLACE_REST_FILE:
			replace_in_file(hwnd,lb_str,FALSE);
			set_replace_rest_file(TRUE);
			EndDialog(hwnd,0);
			break;
		case IDC_REPLACE_ALL:
			replace_in_file(hwnd,lb_str,FALSE);
			set_replace_all_remain(TRUE);
			EndDialog(hwnd,0);
			break;
		case IDOK:
		case IDCANCEL:
		case IDC_CANCEL_REMAINING:
			set_cancel_all(TRUE);
			EndDialog(hwnd,0);
		}
		break;
	}
	return 0;
}


int replace_dlg(HWND hwnd,int lb_index)
{
	int len;
	if(lb_index<0)
		return 0;
	main_hwnd=GetParent(hwnd);
	len=SendDlgItemMessage(main_hwnd,IDC_LIST1,LB_GETTEXTLEN,lb_index,0);
	if(len>0 && len<sizeof(lb_str)){
		lb_str[0]=0;
		len=SendDlgItemMessage(main_hwnd,IDC_LIST1,LB_GETTEXT,lb_index,lb_str);
		if(len>0){
			replaced_str[0]=0;
			create_replace_str(lb_str,replaced_str,sizeof(replaced_str));

			if(get_replace_all_remain() || get_replace_rest_file())
				return replace_in_file(main_hwnd,lb_str,FALSE);
			else if(get_skip_rest_file())
				return TRUE;
			else
				return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_REPLACE),hwnd,replace_proc);
		}
	}
	return 0;
}