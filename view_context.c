#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include <process.h>
#include <float.h>
#include <math.h>
#include "resource.h"
extern HINSTANCE	ghinstance;

int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);


__int64 start_offset=0;
__int64 start_line=0;
__int64 current_line=0;
__int64 last_offset=0;
int binary=FALSE;
WCHAR fname[SR_MAX_PATH];
__int64 fsize=0;
FILE *gfh=0;
int line_count=0;
int scroll_pos=0;
WNDPROC orig_edit=0;
WNDPROC orig_scroll=0;
int edit_sel_pos=0;

int get_file_size(FILE *f,__int64 *s)
{
	_fseeki64(f,0,SEEK_END);
	s[0]=_ftelli64(f);
	return TRUE;
}
static int close_file(FILE **f)
{
	if(f!=0 && ((*f)!=0)){
		last_offset=_ftelli64(*f);
		fclose(*f);
		*f=0;
		return TRUE;
	}
	else
		return FALSE;
}
static int open_file(FILE **f)
{
	if(f!=0 && ((*f)==0)){
		WCHAR tmp[SR_MAX_PATH];
		_snwprintf(tmp,sizeof(tmp)/sizeof(WCHAR),L"\\\\?\\%s",fname);
		tmp[sizeof(tmp)/sizeof(WCHAR)-1]=0;
		*f=_wfopen(tmp,L"rb");
		if(*f!=0)
			_fseeki64(*f,last_offset,SEEK_SET);
	}
	return f!=0 ? TRUE:FALSE;
}
int add_line(HWND hwnd,int ctrl,char *line)
{
	int len;
	HWND h=GetDlgItem(hwnd,ctrl);
	len=GetWindowTextLength(h);
	SendMessage(h,EM_SETSEL,len,len);
	SendMessage(h,EM_REPLACESEL,FALSE,line);
	return TRUE;
}
int set_scroll_pos(HWND hwnd,int ctrl,FILE *f)
{
	int result=FALSE;
	double p;
	__int64 size,offset;
	int pos;
	if(f==0)
		return result;
	offset=_ftelli64(f);
	_fseeki64(f,0,SEEK_END);
	size=_ftelli64(f);
	if(size==0){
		SendDlgItemMessage(hwnd,ctrl,SBM_SETPOS,0,TRUE);
		return result;
	}
	_fseeki64(f,offset,SEEK_SET);
	p=(double)offset/(double)size;
	p*=10000;
	pos=(int)floor(p);
	if(pos>10000)
		pos=10000;
	scroll_pos=pos;
	SendDlgItemMessage(hwnd,ctrl,SBM_SETPOS,pos,TRUE);
	result=TRUE;
	return result;
}
int seek_line_relative(FILE *f,int lines,int dir)
{
	__int64 offset;
	int count=0,result,limit;
	char buf[4];
	offset=_ftelli64(f);
	if(binary){
		if(dir<0)
			offset-=(lines-1)*16;
		else if(dir>0)
			offset+=lines*16;
		if(offset>fsize)
			offset=fsize;
		if(offset<0)
			offset=0;
		_fseeki64(f,offset,SEEK_SET);
		if(dir<0 && lines>1)
			lines--;
		return lines;
	}
	if(offset>fsize)
		offset=fsize;
	limit=0;
	while(offset>=0){
		if(dir>0)
			offset++;
		else if(dir<0)
			offset--;
		if(offset>fsize)
			break;
		result=_fseeki64(f,offset,SEEK_SET);
		if(result!=0)
			break;
		buf[0]=0;
		result=fread(buf,1,1,f);
		if(result<=0)
			break;
		if(buf[0]=='\n'){
			count++;
		}
		if(count>=lines){
			if(buf[0]=='\n')
				_fseeki64(f,offset+1,SEEK_SET);
			if(dir==-1 && count>0)
				count--;
			break;
		}
		if(offset<=0){
			_fseeki64(f,0,SEEK_SET);
			break;
		}
		limit++;
		if(limit>0x4000)
			break;
	}
	return count;
}
int sanitize_str(unsigned char *buf,int len)
{
	int i,result=FALSE;
	for(i=0;i<len;i++){
		if(buf[i]==0)
			result=TRUE;
		if(buf[i]>0x7E)
			result=TRUE;
		if(buf[i]=='\r'||buf[i]=='\n'||buf[i]=='\t')
			continue;
		if(buf[i]<' ' || buf[i]>0x7E)
			buf[i]='.';
	}
	return result;
}
int fill_line_col(HWND hwnd,int ctrl,__int64 line)
{
	char str[20];
	int i;
	SetDlgItemText(hwnd,ctrl,"");
	for(i=0;i<line_count;i++){
		char *s="";
		if(binary){
			__int64 offset=line+i*16;
			if(start_offset>=offset && start_offset<offset+16)
				s="-->";
			else if(offset>=current_line && offset<current_line+16)
				s=".";
			_snprintf(str,sizeof(str),"%s%04I64X\r\n",s,line+i*16);
		}
		else{
			if(start_line==line+i)
				s="-->";
			else if(current_line==line+i)
				s=".";
			_snprintf(str,sizeof(str),"%s%I64i\r\n",s,line+i);
		}
		str[sizeof(str)-1]=0;
		add_line(hwnd,ctrl,str);
	}
	return TRUE;
}
int format_hex_str(char *in,int ilen,char *out,int olen)
{
	int i,index=0;
	char *hex="0123456789ABCDEF";

	for(i=0;i<ilen;i++){
		if(index>olen)break;
		out[index++]=hex[(in[i]>>4)&0xF];
		out[index++]=hex[in[i]&0xF];
		if((i<ilen-1) && (i!=0) && (((i+1)%4)==0))
			out[index++]='.';
		else
			out[index++]=' ';
	}
	out[index++]=' ';
	for(i=0;i<ilen;i++){
		if(index>olen)break;
		if(in[i]>=' ' && in[i]<0x7F)
			out[index++]=in[i];
		else
			out[index++]='.';
	}
	out[index++]='\r';
	out[index++]='\n';
	out[index++]=0;
	return index;
}
int purge_string(FILE *f,int max_len)
{
	int i=0;
	while(i<max_len){
		char buf[256];
		int buf_size=sizeof(buf);
		int len;
		unsigned __int64 pos;
		pos=_ftelli64(f);
		if(fgets(buf,buf_size-1,f)==0)
			break;
		pos=_ftelli64(f)-pos;
		len=(int)pos;
		if(len==0)
			break;
		i+=len;
		if(buf[len-1]=='\n')
			break;
	}
	return i;
}
int fill_context(HWND hwnd,int ctrl,FILE *f)
{
	char buf[512];
	int i,len,count;
	__int64 offset,line;
	offset=_ftelli64(f);

	if(binary){
		current_line=offset;
		line=offset-(line_count/2)*16;
		if(line<0)line=0;
		_fseeki64(f,line,SEEK_SET);
		fill_line_col(hwnd,IDC_ROWNUMBER,line);
	}
	else{
		count=seek_line_relative(f,line_count/2,-1);
		line=current_line-count;
		if(line<=0)
			line=1;
		if(current_line<=0)
			current_line=1;
		fill_line_col(hwnd,IDC_ROWNUMBER,line);
	}

	SendDlgItemMessage(hwnd,ctrl,EM_GETSEL,&edit_sel_pos,0);
	SetDlgItemText(hwnd,ctrl,"");
	for(i=0;i<line_count;i++){
		buf[0]=0;
		if(binary){
			len=fread(buf,1,16,f);
			if(len<=0)
				break;
			buf[16]=0;
			format_hex_str(buf,len,buf+16,sizeof(buf)-16);
			strncpy(buf,buf+16,sizeof(buf));
			len=strlen(buf);
		}
		else{
			__int64 pos=0;
			int buf_size=sizeof(buf);
			pos=_ftelli64(f);
			if(fgets(buf,buf_size-1,f)==0)
				break;
			buf[buf_size-1]=0;
			pos=_ftelli64(f)-pos;
			len=(int)pos;
			if(len>buf_size-1)
				len=buf_size-1;
			sanitize_str(buf,len);
			if(buf[len-1]!='\n'){
				char *s=buf+buf_size-3;
				purge_string(f,0x10000);
				if(len>buf_size-3)
					s=buf+buf_size-3;
				else
					s=buf+len-3;
				s[0]='\r';
				s[1]='\n';
				s[2]=0;

			}
		}
		add_line(hwnd,ctrl,buf);
	}
	SendDlgItemMessage(hwnd,ctrl,EM_SETSEL,edit_sel_pos,edit_sel_pos);
	_fseeki64(f,offset,SEEK_SET);
	return TRUE;
}
int set_context_font(HWND hwnd)
{
	int font;
	char key[80];
	char font_name[80];
	if(binary)
		font=ANSI_FIXED_FONT;
	else{
		key[0]=0;
		get_dropdown_name(IDC_LISTBOX_FONT,key,sizeof(key));
		font_name[0]=0;
		get_ini_str("OPTIONS",key,font_name,sizeof(font_name));
		font=fontname_to_int(font_name);
	}
	SendDlgItemMessage(hwnd,IDC_ROWNUMBER,WM_SETFONT,GetStockObject(font),0);
	SendDlgItemMessage(hwnd,IDC_CONTEXT,WM_SETFONT,GetStockObject(font),0);
	return TRUE;
}
int get_number_of_lines(HWND hwnd,int ctrl)
{
	int i;
	SetDlgItemText(hwnd,ctrl,"");
	for(i=0;i<250;i++){
		add_line(hwnd,ctrl,"\r\n");
	}
	i=SendDlgItemMessage(hwnd,ctrl,EM_GETLINECOUNT,0,0);
	if(i<=0)
		i=10;
	return i;
}
int do_scroll_proc(HWND hwnd,int lines,int dir,int update_pos)
{
	int delta;
	if(open_file(&gfh)){
		delta=seek_line_relative(gfh,lines,dir);
		if(dir>0)
			current_line+=delta;
		else if(dir<0)
			current_line-=delta;
		fill_context(hwnd,IDC_CONTEXT,gfh);
		if(update_pos)
			set_scroll_pos(hwnd,IDC_CONTEXT_SCROLLBAR,gfh);
		return close_file(&gfh);
	}
	else{
		WCHAR str[MAX_PATH*2]={0};
		_snwprintf(str,sizeof(str)/sizeof(WCHAR),L"Cant open %s",fname);
		str[sizeof(str)/sizeof(WCHAR)-1]=0;
		MessageBoxW(hwnd,str,L"error",MB_OK);
		return FALSE;
	}
}
LRESULT APIENTRY subclass_scroll(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("s");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_GETDLGCODE:
		switch(wparam){
		case VK_RETURN:
		case VK_F9:
			binary=!binary;
			set_context_font(GetParent(hwnd));
			do_scroll_proc(GetParent(hwnd),0,0,TRUE);
			break;
		}
		break;
	}
	return CallWindowProc(orig_scroll,hwnd,msg,wparam,lparam); 
}

LRESULT APIENTRY subclass_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	int lines,dir,do_scroll=FALSE;
	int do_proc=TRUE;
#ifdef _DEBUG
	if(FALSE)
//	if(message!=0x200&&message!=0x84&&message!=0x20&&message!=WM_ENTERIDLE)
	if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_DRAWITEM
		&&msg!=WM_CTLCOLORBTN&&msg!=WM_CTLCOLOREDIT&&msg!=WM_CTLCOLORSCROLLBAR)
	//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("e");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}
#endif
	switch(msg){
	case WM_MOUSEWHEEL:
		{
			short wheel=HIWORD(wparam);
			int flags=LOWORD(wparam);
			if(wheel>0){
				dir=-1;
				if(flags&MK_RBUTTON)
					lines=line_count-2;
				else
					lines=3+1;
			}
			else{
				dir=1;
				if(flags&MK_RBUTTON)
					lines=line_count-2-1;
				else
					lines=3;
			}
			do_scroll=TRUE;
			do_proc=FALSE;
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		case VK_F5:
			lines=dir=0;
			do_scroll=TRUE;
			break;
		case VK_RETURN:
		case VK_F9:
			binary=!binary;
			lines=dir=0;
			do_scroll=TRUE;
			set_context_font(GetParent(hwnd));
			break;
		case VK_HOME:
			if(GetKeyState(VK_CONTROL)&0x8000){
				last_offset=0;
				current_line=1;
			}
			else{
				last_offset=start_offset; //_fseeki64(f,start_offset,SEEK_SET);
				current_line=start_line;
			}
			lines=dir=0;
			do_scroll=TRUE;
			break;
		case VK_PRIOR:
			dir=-1;
			lines=line_count-2;
			if(lines<=0)
				lines=1;
			do_scroll=TRUE;
			break;
		case VK_NEXT:
			dir=1;
			lines=line_count-2-1;
			if(lines<=0)
				lines=1;
			do_scroll=TRUE;
			break;
		case VK_LEFT:
		case VK_UP:
			{
				int ctrl=GetKeyState(VK_CONTROL)&0x8000;
				int shift=GetKeyState(VK_SHIFT)&0x8000;
				if(ctrl){
					dir=-1;
					if(shift)
						lines=10;
					else
						lines=2;
					do_scroll=TRUE;
				}
			}
			break;
		case VK_RIGHT:
		case VK_DOWN:
			{
				int ctrl=GetKeyState(VK_CONTROL)&0x8000;
				int shift=GetKeyState(VK_SHIFT)&0x8000;
				if(ctrl){
					dir=1;
					if(shift)
						lines=10;
					else
						lines=1;
					do_scroll=TRUE;
				}
			}
			break;
		}
		break;
	}
	if(do_scroll)
		do_scroll_proc(GetParent(hwnd),lines,dir,TRUE);
	if(do_proc)
		return CallWindowProc(orig_edit,hwnd,msg,wparam,lparam); 
	else
		return 0;
}
int context_help(HWND hwnd)
{
	static int help_active=FALSE;
	if(!help_active){
		help_active=TRUE;
		MessageBox(hwnd,"F9/ENTER=change between text/hex mode\r\n","HELP",MB_OK);
		help_active=FALSE;
	}
	return help_active;
}
LRESULT CALLBACK view_context_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	int lines,dir,do_scroll=FALSE,update_scroll_pos=TRUE;
	static int divider_drag=FALSE,org_row_width=90,row_width=90,last_pos=0;
#ifdef _DEBUG
	if(FALSE)
//	if(message!=0x200&&message!=0x84&&message!=0x20&&message!=WM_ENTERIDLE)
//	if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_DRAWITEM
//		&&msg!=WM_CTLCOLORBTN&&msg!=WM_CTLCOLOREDIT&&msg!=WM_CTLCOLORSCROLLBAR)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("v");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}
#endif	
	switch(msg)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd,IDC_CONTEXT_SCROLLBAR,SBM_SETRANGE,0,10000);
		{
			int tabstop=21; //4 fixed chars
			SendDlgItemMessage(hwnd,IDC_CONTEXT,EM_SETTABSTOPS,1,&tabstop);
		}
		set_context_font(hwnd);
		open_file(&gfh);
		if(gfh==0){
			WCHAR str[MAX_PATH*2];
			_snwprintf(str,sizeof(str)/sizeof(WCHAR),L"cant open %s",fname);
			str[sizeof(str)/sizeof(WCHAR)-1]=0;
			MessageBoxW(hwnd,str,L"error",MB_OK);
			EndDialog(hwnd,0);
			return 0;
		}
		get_file_size(gfh,&fsize);
		_fseeki64(gfh,start_offset,SEEK_SET);
		set_scroll_pos(hwnd,IDC_CONTEXT_SCROLLBAR,gfh);
		SetFocus(GetDlgItem(hwnd,IDC_CONTEXT_SCROLLBAR));
		get_ini_value("CONTEXT_SETTINGS","row_width",&row_width);
		set_context_divider(row_width);
		line_count=get_number_of_lines(hwnd,IDC_CONTEXT);
		fill_context(hwnd,IDC_CONTEXT,gfh);
		close_file(&gfh);
		last_pos=-1;
		orig_edit=SetWindowLong(GetDlgItem(hwnd,IDC_CONTEXT),GWL_WNDPROC,subclass_edit);
		orig_scroll=SetWindowLong(GetDlgItem(hwnd,IDC_CONTEXT_SCROLLBAR),GWL_WNDPROC,subclass_scroll);
		SetWindowTextW(hwnd,fname);
		grippy=create_grippy(hwnd);
		return 0;
	case WM_HELP:
		context_help(hwnd);
		return TRUE;
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		line_count=get_number_of_lines(hwnd,IDC_CONTEXT);
		open_file(&gfh);
		fill_context(hwnd,IDC_CONTEXT,gfh);
		set_scroll_pos(hwnd,IDC_CONTEXT_SCROLLBAR,gfh);
		close_file(&gfh);
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		ReleaseCapture();
		if(divider_drag){
			write_ini_value("CONTEXT_SETTINGS","row_width",row_width);
			divider_drag=FALSE;
		}
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));
		divider_drag=TRUE;
		org_row_width=row_width;
		break;
	case WM_MOUSEFIRST:
		{
			int x=LOWORD(lparam);
			SetCursor(LoadCursor(NULL,IDC_SIZEWE));
			if(divider_drag){
				RECT rect;
				GetClientRect(hwnd,&rect);
				if((rect.right-x)>25 && x>5){
					row_width=x;
					set_context_divider(row_width);
				}
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			short wheel=HIWORD(wparam);
			int flags=LOWORD(wparam);
			if(wheel>0){
				dir=-1;
				if(flags&MK_RBUTTON)
					lines=line_count-2;
				else
					lines=3+1;
			}
			else{
				dir=1;
				if(flags&MK_RBUTTON)
					lines=line_count-2-1;
				else
					lines=3;
			}
			do_scroll=TRUE;
		}
		break;
	case WM_VSCROLL:
		{
		int pos=HIWORD(wparam);
		switch(LOWORD(wparam)){
		case SB_TOP:
			if(GetKeyState(VK_CONTROL)&0x8000){
				last_offset=0;
				current_line=1;
			}
			else{
				last_offset=start_offset; //_fseeki64(f,start_offset,SEEK_SET);
				current_line=start_line;
			}
			lines=dir=0;
			do_scroll=TRUE;
			break;
		case SB_PAGEUP:
			dir=-1;
			lines=line_count-2;
			if(lines<=0)
				lines=1;
			do_scroll=TRUE;
			break;
		case SB_PAGEDOWN:
			dir=1;
			lines=line_count-2-1;
			if(lines<=0)
				lines=1;
			do_scroll=TRUE;
			break;
		case SB_LINEUP:
			dir=-1;
			lines=2;
			do_scroll=TRUE;
			break;
		case SB_LINEDOWN:
			dir=1;
			lines=1;
			do_scroll=TRUE;
			break;
		case SB_THUMBTRACK:
			//printf("pos=%i last_pos=%i scroll_pos=%i line_count=%i\n",HIWORD(wparam),last_pos,scroll_pos,line_count);
			if(pos<last_pos){
				dir=-1;
				lines=line_count/4;
				if(lines<=1)
					lines=2;
				do_scroll=TRUE;
			}
			else if(pos>last_pos){
				dir=1;
				lines=line_count/4;
				if(lines==0)
					lines=1;
				do_scroll=TRUE;
			}
			if(last_pos==-1)
				do_scroll=FALSE;
			last_pos=pos;
			update_scroll_pos=FALSE;
			break;
		case SB_THUMBPOSITION: //dragged and released
			dir=lines=0;
			do_scroll=TRUE;
			break;
		case SB_ENDSCROLL:
			last_pos=-1;
			break;
		}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDCANCEL:
			if(divider_drag){
				divider_drag=FALSE;
				ReleaseCapture();
				set_context_divider(org_row_width);
				row_width=org_row_width;
				return 0;
			}
			if(gfh!=0)
				fclose(gfh);
			gfh=0;
			if(orig_edit!=0)SetWindowLong(GetDlgItem(hwnd,IDC_CONTEXT),GWL_WNDPROC,orig_edit);
			EndDialog(hwnd,0);
			return 0;
		}
		break;
	}
	if(do_scroll)
		do_scroll_proc(hwnd,lines,dir,update_scroll_pos);
	return 0;
}


int view_context(HWND hwnd)
{
	char *str=0;
	int index,len;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(index>=0){
		int found_line=FALSE;
		binary=FALSE;
		len=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,index,0);
		if(len==LB_ERR)
			len=0x10000;
		str=malloc(len+1);
		if(str!=0){
			str[0]=0;
			start_offset=start_line=0;
			found_line=TRUE;
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,index,str);
			if(strnicmp(str,"line",sizeof("line")-1)==0) //"Line %I64i col %I64i = %i %i %I64X -%s"
				sscanf(str,"%*s %I64i %*s %*s %*s %*s %*s %I64X",&start_line,&start_offset);
			else if(strnicmp(str,"offset",sizeof("offset")-1)==0){ //"Offset 0x%I64X = %i %i -%s"
				sscanf(str,"%*s 0x%I64x",&start_offset);
				binary=TRUE;
			}
			else if(strnicmp(str,"file",sizeof("file")-1)==0)
				found_line=TRUE;
			else
				found_line=FALSE;
			current_line=start_line;
			free(str);
		}
		fname[0]=0;
		get_nearest_filename(hwnd,fname,sizeof(fname)/sizeof(WCHAR));
		if(found_line && fname[0]!=0)
			return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_VIEWCONTEXT),hwnd,view_context_proc);
	}
	return FALSE;
}

int open_url_listview(HWND hwnd)
{
	int index,len;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(index>=0){
		char *str=0;
		int found_url=FALSE;
		len=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,index,0);
		if(len==LB_ERR)
			len=0x10000;
		str=malloc(len+1);
		if(str!=0){
			unsigned char *url;
			str[0]=0;
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,index,str);
			str[len]=0;
			url=strstri(str,"http://");
			if(url==0)
				url=strstri(str,"https://");
			if(url){
				int i,max;
				max=strlen(url);
				for(i=0;i<max;i++){
					if(url[i]<=' '){
						url[i]=0;
						break;
					}
				}
				ShellExecute(NULL,"open",url,NULL,NULL,SW_SHOWNORMAL);
				found_url=TRUE;
			}
			free(str);
		}
		if(!found_url)
			view_context(hwnd);
	}
	return FALSE;
}