#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define COMPILE_MULTIMON_STUBS
#endif
#include <windows.h>
#include <multimon.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <fcntl.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include "resource.h"

HWND		hwindow;
HINSTANCE	ghinstance;
int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);
char *cmdline=0;
static HMENU list_menu=0;

enum{
	CMD_OPENASSOC=10000,
	CMD_OPENWITH,
	CMD_SEARCH_THIS_FILE=10000+100,
	CMD_SEARCH_THIS_FILE_TYPE,
	CMD_SEARCH_ALL_FILE,
	CMD_SEARCH_THIS_FOLDER,
	CMD_END
};
int move_console()
{
	BYTE Title[200]; 
	HANDLE hConWnd; 
	GetConsoleTitle(Title,sizeof(Title));
	hConWnd=FindWindow(NULL,Title);
	SetWindowPos(hConWnd,0,650,0,0,0,SWP_NOSIZE|SWP_NOZORDER);
	return 0;
}
void open_console()
{
	char title[MAX_PATH]={0}; 
	HWND hcon; 
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;
	
	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole(); 
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w"); 
	*stdout=*hf; 
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW); 
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}
void hide_console()
{
	char title[MAX_PATH]={0}; 
	HANDLE hcon; 
	
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_HIDE);
		SetForegroundWindow(hcon);
	}
}

int get_filename(char *path,char *fname,int size)
{
	char drive[4];
	char dir[255];
	char name[255];
	char ext[255];
	_splitpath(path,drive,dir,name,ext);
	_snprintf(fname,size,"%s%s",name,ext);
	return TRUE;
}

int show_main_help(HWND hwnd,HELPINFO *hi)
{
	static int help_active=FALSE;
	if(!help_active){
		help_active=TRUE;
		switch(hi->iCtrlId){
		default:
		case IDC_LIST1:
			MessageBox(hwnd,
				"F5=Search\r\n"
				"F6=Replace\r\n"
				"F9 / ctrl+o=open options\r\n"
				"ALT+HOME = always on top\r\n"
				"double click on line to open in context viewer\r\n"
				"[1-9] open with app set in options\r\n"
				"ctrl+a select all lines\r\n"
				"ctrl+c copy lines\r\n"
				"ctrl+shift+c copy lines with all info\r\n"
				"ctrl+s=set search focus\r\n"
				"ctrl+r=set replace focus\r\n"
				"ctrl+f=set file mask focus\r\n"
				"ctrl+p=set path focus\r\n\r\n"
				"ctrl+drag=mask exact filenames\r\n"
				"shift+drag=mask by *.*\r\n"
				"ctrl+shift+drag=mask by *.ext\r\n"
				,"HELP",MB_OK);
			break;
		case IDC_COMBO_PATH:
			MessageBox(hwnd,
				"seperate paths with ;\r\n"
				,"HELP",MB_OK);
			break;
		case IDC_FILE_OPTIONS:
		case IDC_COMBO_MASK:
			MessageBox(hwnd,
				"wildcards *,?\r\n"
				"~=exclude\r\n"
				"\\=path ex:\\*test* matches all paths that contain test\r\n"
				"seperate mask with ;\r\n"
				,"HELP",MB_OK);
			break;
		}
		help_active=FALSE;
	}
	return help_active;
}
int check_path_added(char *paths,char *np)
{
	int len;
	char *p;
	p=strstr(paths,np);
	if(p==0)
		return FALSE;
	len=strlen(np);
checknext:
	if(p[len]==0 || p[len]==';')
		return TRUE;
	p=strstr(p+1,np);
	if(p!=0)
		goto checknext;
	return FALSE;
}
int process_drop(HWND hwnd,HANDLE hdrop,int ctrl,int shift)
{
	int i,count;
	char str[MAX_PATH];
	char paths[1024]={0},mask[1024]={0};
	count=DragQueryFile(hdrop,-1,NULL,0);
	for(i=0;i<count;i++){
		str[0]=0;
		DragQueryFile(hdrop,i,str,sizeof(str));
		if(is_path_directory(str)){
			if(strlen(mask)<(sizeof(mask)-5)){
				if(strstr(mask,"*.*")==0){
					if(mask[0]!=0)
						strcat(mask,";");
					strcat(mask,"*.*");
				}
			}
			if(!check_path_added(paths,str)){
				if((strlen(paths)+strlen(str)+1)<sizeof(paths)){
					if(paths[0]!=0)
						strcat(paths,";");
					strcat(paths,str);
				}
			}
		}
		else{ //its a file
			char drive[_MAX_DRIVE],fpath[_MAX_PATH],fname[_MAX_PATH],dir[_MAX_PATH],ext[_MAX_EXT];
			_splitpath(str,drive,dir,fname,ext);
			_snprintf(fpath,sizeof(fpath),"%s%s",drive,dir);
			if(!check_path_added(paths,fpath)){
				if((strlen(paths)+strlen(fpath)+1)<sizeof(paths)){
					if(paths[0]!=0)
						strcat(paths,";");
					strcat(paths,fpath);
				}
			}
			if(shift && (!ctrl)){
				if(strlen(mask)<(sizeof(mask)-5)){
					if(strstr(mask,"*.*")==0){
						if(mask[0]!=0)
							strcat(mask,";");
						strcat(mask,"*.*");
					}
				}
			}
			else if(ctrl && (!shift)){ //mask by exact files
				char f[MAX_PATH]={0};
				_snprintf(f,sizeof(f),"%s%s",fname,ext);
				if(strstr(mask,f)==0){
					if((strlen(mask)+strlen(f)+1)<sizeof(mask)){
						if(mask[0]!=0)
							strcat(mask,";");
						strcat(mask,f);
					}
				}
			}
			else if(ext[0]!=0 && strstr(mask,ext)==0){
				if((strlen(mask)+strlen(ext)+1)<sizeof(mask)){
					if(mask[0]!=0)
						strcat(mask,";");
					_snprintf(mask,sizeof(mask),"%s*%s",mask,ext);
				}
			}

		}
	}
	DragFinish(hdrop);
	if(mask[0]!=0){
		int index;
		if(shift || ctrl || 0==GetWindowTextLength(GetDlgItem(hwnd,IDC_COMBO_MASK))){
			index=SendDlgItemMessage(hwnd,IDC_COMBO_MASK,CB_FINDSTRINGEXACT,-1,mask);
			if(index==CB_ERR)
				index=SendDlgItemMessage(hwnd,IDC_COMBO_MASK,CB_ADDSTRING,0,mask);
			if(index!=CB_ERR)
				SendDlgItemMessage(hwnd,IDC_COMBO_MASK,CB_SETCURSEL,index,0);
		}
	}
	if(paths[0]!=0){
		int index;
		index=SendDlgItemMessage(hwnd,IDC_COMBO_PATH,CB_FINDSTRINGEXACT,-1,paths);
		if(index==CB_ERR)
			index=SendDlgItemMessage(hwnd,IDC_COMBO_PATH,CB_ADDSTRING,0,paths);
		if(index!=CB_ERR)
			SendDlgItemMessage(hwnd,IDC_COMBO_PATH,CB_SETCURSEL,index,0);
	}
	return 0;
}
int find_valid_path(char *path)
{
	int i;
	i=strlen(path);
check:
	if(is_path_directory(path))
		return TRUE;
	while(i>2){
		i--;
		if(path[i]=='\\'){
			path[i+1]=0;
			goto check;
		}
	}
	return FALSE;
}
int CALLBACK BrowseCallbackProc(HWND hwnd,UINT msg,LPARAM lparam,LPARAM lpdata)
{
	static int init=FALSE;
	switch(msg){
	case BFFM_INITIALIZED:
		SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpdata);
		init=TRUE;
		break;
	case BFFM_SELCHANGED:
		if(init)
		{
			HTREEITEM h;
			HWND tree = GetDlgItem(GetDlgItem(hwnd, 0), 0x64);
			h=TreeView_GetSelection(tree);
			TreeView_EnsureVisible(tree,h);
			init=FALSE;
		}
		break;
	}
	return 0;
}
int handle_browse_dialog(HWND hwnd,char *path)
{
	ITEMIDLIST *pidl;
	BROWSEINFO bi;
	IMalloc	*palloc;
	int result=FALSE;
	memset(&bi,0,sizeof(bi));
	bi.hwndOwner=hwnd;
	bi.ulFlags=BIF_EDITBOX|0x40; //BIF_NEWDIALOGSTYLE
	if(path[0]!=0){
		if(find_valid_path(path)){
			bi.lpfn=BrowseCallbackProc;
			bi.lParam=(long)path;
		}
	}
	pidl=SHBrowseForFolder(&bi);
	if(pidl!=0){
		result=SHGetPathFromIDList(pidl,path);
		if(SHGetMalloc(&palloc)==NOERROR)
		{
			palloc->lpVtbl->Free(palloc,pidl);
			palloc->lpVtbl->Release(palloc);
		}

	}
	return result;
}
static int set_fonts(HWND hwnd)
{
	int font;
	char key[80];
	char font_name[80];
	key[0]=0;
	get_dropdown_name(IDC_COMBO_FONT,key,sizeof(key));
	font_name[0]=0;
	get_ini_str("OPTIONS",key,font_name,sizeof(font_name));
	font=fontname_to_int(font_name);
	SendDlgItemMessage(hwnd,IDC_COMBO_SEARCH,WM_SETFONT,GetStockObject(font),0);
	SendDlgItemMessage(hwnd,IDC_COMBO_REPLACE,WM_SETFONT,GetStockObject(font),0);
	SendDlgItemMessage(hwnd,IDC_COMBO_PATH,WM_SETFONT,GetStockObject(font),0);
	SendDlgItemMessage(hwnd,IDC_COMBO_MASK,WM_SETFONT,GetStockObject(font),0);
	key[0]=0;
	get_dropdown_name(IDC_LISTBOX_FONT,key,sizeof(key));
	font_name[0]=0;
	get_ini_str("OPTIONS",key,font_name,sizeof(font_name));
	font=fontname_to_int(font_name);
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,GetStockObject(font),0);
	return TRUE;
}
int is_button_checked(HWND hwnd,int control)
{
	if(BST_CHECKED&SendDlgItemMessage(hwnd,control,BM_GETSTATE,0,0))
		return TRUE;
	else
		return FALSE;
}
int load_combo_box(HWND hwnd,int ctrl,char *keystart)
{
	char str[1024];
	char key[80];
	int i;
	str[0]=0;
	get_ini_str("COMBO_BOXES",keystart,str,sizeof(str));
	SetDlgItemText(hwnd,ctrl,str);
	for(i=0;i<get_max_items_combobox();i++){
		_snprintf(key,sizeof(key),"%s%i",keystart,i+1);
		str[0]=0;
		get_ini_str("COMBO_BOXES",key,str,sizeof(str));
		if(strlen(str)>0)
			SendDlgItemMessage(hwnd,ctrl,CB_INSERTSTRING,-1,str);
	}
	return TRUE;
}

int get_ini_stuff(HWND hwnd)
{
	int val;
	load_combo_box(hwnd,IDC_COMBO_SEARCH,"search");
	load_combo_box(hwnd,IDC_COMBO_REPLACE,"replace");
	load_combo_box(hwnd,IDC_COMBO_MASK,"mask");
	load_combo_box(hwnd,IDC_COMBO_PATH,"path");
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","search_subdirs",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_SUBDIRS,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","unicode",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_UNICODE,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","case",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_CASE,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","wholeword",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_WHOLEWORD,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","hex",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_HEX,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","wildcard",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_WILDCARD,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","ignore_whitespace",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_IGNOREWS,BM_SETCHECK,BST_CHECKED,0);
	val=FALSE;
	get_ini_value("BUTTON_SETTINGS","on_top",&val);
	if(val)
		SendDlgItemMessage(hwnd,IDC_ONTOP,BM_SETCHECK,BST_CHECKED,0);
	return TRUE;
}
int save_combo_box(HWND hwnd,int ctrl,char *keystart)
{
	char str[1024];
	char key[80];
	int i;
	str[0]=0;
	GetDlgItemText(hwnd,ctrl,str,sizeof(str));
	if(strcmp(str,"Binary mode -->>")!=0)
		write_ini_str("COMBO_BOXES",keystart,str);
	for(i=0;i<get_max_items_combobox();i++){
		int len;
		len=SendDlgItemMessage(hwnd,ctrl,CB_GETLBTEXTLEN,i,0);
		if(len>0 && len<=sizeof(str)-1){
			str[0]=0;
			SendDlgItemMessage(hwnd,ctrl,CB_GETLBTEXT,i,str);
			if(strlen(str)>0){
				_snprintf(key,sizeof(key),"%s%i",keystart,i+1);
				write_ini_str("COMBO_BOXES",key,str);
			}
		}
	}
	return TRUE;
}
int save_ini_stuff(HWND hwnd)
{
	write_ini_value("BUTTON_SETTINGS","search_subdirs",is_button_checked(hwnd,IDC_SUBDIRS));
	write_ini_value("BUTTON_SETTINGS","unicode",is_button_checked(hwnd,IDC_UNICODE));
	write_ini_value("BUTTON_SETTINGS","case",is_button_checked(hwnd,IDC_CASE));
	write_ini_value("BUTTON_SETTINGS","wholeword",is_button_checked(hwnd,IDC_WHOLEWORD));
	write_ini_value("BUTTON_SETTINGS","hex",is_button_checked(hwnd,IDC_HEX));
	write_ini_value("BUTTON_SETTINGS","wildcard",is_button_checked(hwnd,IDC_WILDCARD));
	write_ini_value("BUTTON_SETTINGS","ignore_whitespace",is_button_checked(hwnd,IDC_IGNOREWS));
	write_ini_value("BUTTON_SETTINGS","on_top",is_button_checked(hwnd,IDC_ONTOP));
	save_combo_box(hwnd,IDC_COMBO_SEARCH,"search");
	save_combo_box(hwnd,IDC_COMBO_REPLACE,"replace");
	save_combo_box(hwnd,IDC_COMBO_MASK,"mask");
	save_combo_box(hwnd,IDC_COMBO_PATH,"path");
	return TRUE;
}
int set_status_bar(HWND hwnd,char *fmt,...)
{
	char str[1024]={0};
	va_list args;
	va_start(args,fmt);
	_vsnprintf(str,sizeof(str),fmt,args);
	str[sizeof(str)-1]=0;
	SetDlgItemText(hwnd,IDC_STATUS,str);
	return TRUE;
}
int copy_str_clipboard(char *str)
{
	int len,result=FALSE;
	HGLOBAL hmem;
	char *lock;
	len=strlen(str);
	if(len==0)
		return result;
	len++;
	hmem=GlobalAlloc(GMEM_MOVEABLE,len);
	if(hmem!=0){
		lock=GlobalLock(hmem);
		if(lock!=0){
			memcpy(lock,str,len);
			GlobalUnlock(hmem);
			if(OpenClipboard(NULL)!=0){
				EmptyClipboard();
				SetClipboardData(CF_TEXT,hmem);
				CloseClipboard();
				result=TRUE;
			}
		}
		if(!result)
			GlobalFree(hmem);
	}
	return result;
}

int copy_items_clip(HWND hwnd,int copy_all)
{
	int count;
	int *list;
	count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSELCOUNT,0,0);
	if(count>0){
		count&=0x1FFFF;
		list=malloc(sizeof(int)*count);
		if(list!=0){
			int items;
			memset(list,0,sizeof(int)*count);
			items=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETSELITEMS,count,list);
			if(items>0){
				int i;
				unsigned int size=0;
				char *buf=0;
				for(i=0;i<items;i++){
					int len;
					len=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,list[i],0);
					if(len>0){
						char *tbuf=0;
						tbuf=realloc(buf,size+len+2+1); //CRLF+null
						if(tbuf!=0){
							buf=tbuf;
							if(SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,list[i],buf+size)>0){
								if(copy_all){
copyall:
									if(i<items-1)
										strcat(buf+size,"\r\n");
									else
										buf[size+len]=0;
									size+=strlen(buf+size);
								}else{
									if(strnicmp(buf+size,"line ",sizeof("line ")-1)==0 || 
										strnicmp(buf+size,"offset ",sizeof("offset ")-1)==0){
										char *s=strstr(buf+size,"-");
										if(s!=0){
											int diff=strlen(buf+size)-strlen(s+1);
											if(diff>0 && diff<len)
												strncpy(buf+size,s+1,len+1);
											if(i<items-1)
												strcat(buf+size,"\r\n");
											size+=strlen(buf+size);
										}
									}
									else if(strnicmp(buf+size,"file ",sizeof("file ")-1)==0){
										strncpy(buf+size,buf+size+sizeof("file ")-1,len+1);
										if(i<items-1)
											strcat(buf+size,"\r\n");
										size+=strlen(buf+size);
									}
									else
										goto copyall;
								}
							}
						}
					}
				}
				if(buf!=0){
					copy_str_clipboard(buf);
					free(buf);
				}
			}
			free(list);
		}
	}
	return TRUE;
}
int create_list_menu(HWND hwnd){
	int max_open_with=get_max_open_with();
	if(list_menu!=0)DestroyMenu(list_menu);
	if(list_menu=CreatePopupMenu()){
		int i;
		char key[20],caption[40],str[80];
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_OPENASSOC,"Open with associated");
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		for(i=0;i<max_open_with;i++){
			_snprintf(key,sizeof(key),"caption%i",i+1);
			caption[0]=0;
			get_ini_str("OPTIONS",key,caption,sizeof(caption));
			if(strlen(caption)>0){
				_snprintf(str,sizeof(str),"Open with [%i] %s",i+1,caption);
				InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_OPENWITH+i,str);
			}
		}
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_SEPARATOR,0,0);
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SEARCH_THIS_FILE,"search this file");
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SEARCH_THIS_FILE_TYPE,"search this file type");
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SEARCH_ALL_FILE,"search *.*");
		InsertMenu(list_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SEARCH_THIS_FOLDER,"search this folder");
	}
	return TRUE;
}
int set_listitem_mouse(HWND hwnd)
{
	unsigned int index,count;
	POINT screen={0};
	count=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCOUNT,0,0);
	if(count>0 && count<=65536 ){
		GetCursorPos(&screen);
		ScreenToClient(GetDlgItem(hwnd,IDC_LIST1),&screen);
		index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_ITEMFROMPOINT,0,MAKELPARAM(screen.x,screen.y));
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,FALSE,MAKELPARAM(0,0xFFFF));
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETSEL,TRUE,index);
	}
	return TRUE;
}

int open_with_cmd(HWND hwnd,char *fname,int index,int cmd)
{
	char *buf=0;
	int len;
	__int64 line=0,col=0,offset=0;
	len=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,index,0);
	if(len>0){
		buf=malloc(len+1);
		if(buf!=0){
			char *str=0;
			buf[0]=0;
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,index,buf);
			if(strnicmp(buf,"line",sizeof("line")-1)==0)
				sscanf(buf,"%*s %I64i %*s %I64i %*s %*s %*s %I64x",&line,&col,&offset);//"Line %I64i col %I64i = %i %i %I64X -%s"
			else if(strnicmp(buf,"offset",sizeof("offset")-1)==0)
				//"Offset 0x%I64X = %I64i %i %i -%s"
				sscanf(buf,"%*s 0x%I64x %*s %I64i",&offset,&line);
			str=malloc(MAX_PATH*2);
			if(str!=0){
				int open_num=cmd-CMD_OPENWITH+1;
				char key[20];
				char tmp[40]={0};
				char *out=0;
				str[0]=0;
				if(open_num<0)open_num=0;
				_snprintf(key,sizeof(key),"open%i",open_num);
				get_ini_str("OPTIONS",key,str,MAX_PATH*2);
				pipe_to_quote(str);
				str_replace(str,"%PATH%",fname,&out);
				if(out!=0){free(str);str=out;out=0;}
				_snprintf(tmp,sizeof(tmp),"%I64i",line);
				str_replace(str,"%LINE%",tmp,&out);
				if(out!=0){free(str);str=out;out=0;}
				_snprintf(tmp,sizeof(tmp),"%I64i",col);
				str_replace(str,"%COL%",tmp,&out);
				if(out!=0){free(str);str=out;out=0;}
				_snprintf(tmp,sizeof(tmp),"%I64X",offset);
				str_replace(str,"%OFFSET%",tmp,&out);
				if(out!=0){free(str);str=out;out=0;}
				if(str[0]!=0)
					shell_execute(hwnd,str);
				free(str);
			}
			free(buf);
		}
	}
	return TRUE;
}
int get_nearest_filename(HWND hwnd,char *fname,int flen)
{
	int index,fitem=-1;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(index>=0){
		int i;
		for(i=index;i>=0;i--){
			int len;
			char *buf=0;
			len=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXTLEN,i,0);
			if(len>0){
				buf=malloc(len+1);
				if(buf!=0){
					buf[0]=0;
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,i,buf);
					if(strnicmp(buf,"file",sizeof("file")-1)==0){
						strncpy(fname,buf+sizeof("file ")-1,flen);
						fname[flen-1]=0;
						fitem=i;
					}
					free(buf);
					if(fitem>=0)
						break;
				}
			}
		}
	}
	return fitem;
}
int handle_context_open(HWND hwnd,int cmd)
{
	char fname[MAX_PATH]={0};
	int index;
	if(cmd==CMD_SEARCH_ALL_FILE){
		SetDlgItemText(hwnd,IDC_COMBO_MASK,"*.*");
		return TRUE;
	}
	get_nearest_filename(hwnd,fname,sizeof(fname));
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(fname[0]!=0 && index>=0){
		if(strlen(fname)>0){
			switch(cmd){
			default:
				open_with_cmd(hwnd,fname,index,cmd);
				break;
			case CMD_SEARCH_THIS_FOLDER:
				{
					char drive[_MAX_DRIVE]={0},dir[_MAX_DIR],path[_MAX_PATH];
					_splitpath(fname,drive,dir,NULL,NULL);
					_snprintf(path,sizeof(path),"%s%s",drive,dir);
					if(strlen(path)>0)
						SetDlgItemText(hwnd,IDC_COMBO_PATH,path);
				}
				break;
			case CMD_SEARCH_THIS_FILE_TYPE:
				{
					char ext[_MAX_PATH]={0},mask[MAX_PATH];
					_splitpath(fname,NULL,NULL,NULL,ext);
					if(strlen(ext)>0){
						_snprintf(mask,sizeof(mask),"*%s",ext);
						SetDlgItemText(hwnd,IDC_COMBO_MASK,mask);
					}
				}
				break;
			case CMD_SEARCH_THIS_FILE:
				{
					char f[MAX_PATH]={0};
					char drive[_MAX_DRIVE]={0},dir[_MAX_PATH]={0},path[MAX_PATH]={0};
					get_filename(fname,f,sizeof(f));
					if(strlen(f)>0)
						SetDlgItemText(hwnd,IDC_COMBO_MASK,f);
					_splitpath(fname,drive,dir,NULL,NULL);
					_snprintf(path,sizeof(path),"%s%s",drive,dir);
					if(strlen(path)>0)
						SetDlgItemText(hwnd,IDC_COMBO_PATH,path);

				}
				break;
			case CMD_OPENASSOC:
				ShellExecute(hwnd,"open",fname,NULL,NULL,SW_SHOWNORMAL);
				break;

			}

		}
	}
	return TRUE;
}
int get_nearest_monitor(int x,int y,int width,int height,RECT *rect)
{
	HMONITOR hmon;
	MONITORINFO mi;
	RECT r={0};
	r.left=x;
	r.top=y;
	r.right=x+width;
	r.bottom=y+height;
	hmon=MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST);
    mi.cbSize=sizeof(mi);
	if(GetMonitorInfo(hmon,&mi)){
		*rect=mi.rcWork;
		return TRUE;
	}
	return FALSE;
}
int save_window_pos_relative(HWND hparent,HWND hwnd,char *section)
{
	WINDOWPLACEMENT wp;
	if(GetWindowPlacement(hwnd,&wp)!=0){
		RECT rect={0},rect_parent={0};
		int xpos,ypos,width,height;
		if(wp.flags&WPF_RESTORETOMAXIMIZED)
			write_ini_value(section,"maximized",1);
		else
			write_ini_value(section,"maximized",0);

		rect=wp.rcNormalPosition;
		width=rect.right-rect.left;
		height=rect.bottom-rect.top;
		if(width<100)width=320;
		if(height<100)height=240;
		write_ini_value(section,"width",width);
		write_ini_value(section,"height",height);

		GetWindowRect(hparent,&rect_parent);
		xpos=rect.left-rect_parent.left;
		ypos=rect.top-rect_parent.top;
		if(GetKeyState(VK_SHIFT)&0x8000){
			xpos=ypos=0;
		}
		write_ini_value(section,"xpos",xpos);
		write_ini_value(section,"ypos",ypos);
	}
	return TRUE;
}
int load_window_pos_relative(HWND hparent,HWND hwnd,char *section)
{
	int width=0,height=0,x=0,y=0,maximized=0;
	RECT rect={0};
	int result=FALSE;
	get_ini_value(section,"width",&width);
	get_ini_value(section,"height",&height);
	get_ini_value(section,"xpos",&x);
	get_ini_value(section,"ypos",&y);
	get_ini_value(section,"maximized",&maximized);
	if(get_nearest_monitor(x,y,width,height,&rect)){
		int flags=SWP_SHOWWINDOW;
		if(width<50 || height<50)
			flags|=SWP_NOSIZE;
		if(hparent!=0){
			RECT rect_parent={0};
			GetWindowRect(hparent,&rect_parent);
			x=rect_parent.left+x;
			y=rect_parent.top+y;
			if(x>(rect.right-25) || x<(rect.left-25)
				|| y<(rect.top-25) || y>(rect.bottom-25))
				flags|=SWP_NOMOVE;
		}
		else
			flags|=SWP_NOMOVE;
		if(SetWindowPos(hwnd,HWND_TOP,x,y,width,height,flags)!=0)
			result=TRUE;
	}
	if(maximized)
		PostMessage(hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);
	return result;
}
int load_window_size(HWND hwnd,char *section)
{
	RECT rect={0};
	int width=0,height=0,x=0,y=0,maximized=0;
	int result=FALSE;
	get_ini_value(section,"width",&width);
	get_ini_value(section,"height",&height);
	get_ini_value(section,"xpos",&x);
	get_ini_value(section,"ypos",&y);
	get_ini_value(section,"maximized",&maximized);
	if(get_nearest_monitor(x,y,width,height,&rect)){
		int flags=SWP_SHOWWINDOW;
		if((GetKeyState(VK_SHIFT)&0x8000)==0){
			if(width<50 || height<50)
				flags|=SWP_NOSIZE;
			if(x>(rect.right-25) || x<(rect.left-25)
				|| y<(rect.top-25) || y>(rect.bottom-25))
				flags|=SWP_NOMOVE;
			if(SetWindowPos(hwnd,HWND_TOP,x,y,width,height,flags)!=0)
				result=TRUE;
		}
	}
	if(maximized)
		PostMessage(hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);
	return result;
}
int save_window_size(HWND hwnd,char *section)
{
	WINDOWPLACEMENT wp;
	RECT rect={0};
	int x,y;

	wp.length=sizeof(wp);
	if(GetWindowPlacement(hwnd,&wp)!=0){
		if(wp.flags&WPF_RESTORETOMAXIMIZED)
			write_ini_value(section,"maximized",1);
		else
			write_ini_value(section,"maximized",0);

		rect=wp.rcNormalPosition;
		x=rect.right-rect.left;
		y=rect.bottom-rect.top;
		if(x<100)x=320;
		if(y<100)y=240;
		write_ini_value(section,"width",x);
		write_ini_value(section,"height",y);
		write_ini_value(section,"xpos",rect.left);
		write_ini_value(section,"ypos",rect.top);
	}
	return TRUE;
}
int exclude_buttons(HWND hwnd,int ctrl)
{
	int i;
	typedef struct{
		int ctrl;
		int list[5];
	}EXCLUDE;
	EXCLUDE exc[]={
		{IDC_HEX,{IDC_UNICODE,IDC_WHOLEWORD,IDC_WILDCARD,IDC_IGNOREWS,0}},
		{IDC_UNICODE,{IDC_HEX,IDC_WHOLEWORD,IDC_WILDCARD,IDC_IGNOREWS,0}},
		{IDC_WHOLEWORD,{IDC_HEX,IDC_UNICODE,IDC_WILDCARD,IDC_IGNOREWS,0}},
		{IDC_WILDCARD,{IDC_HEX,IDC_UNICODE,IDC_WHOLEWORD,IDC_IGNOREWS,0}},
		{IDC_IGNOREWS,{IDC_HEX,IDC_UNICODE,IDC_WHOLEWORD,IDC_WILDCARD,0}},
	};
	if(!IsDlgButtonChecked(hwnd,ctrl))
		return FALSE;
	for(i=0;i<sizeof(exc)/sizeof(EXCLUDE);i++){
		if(exc[i].ctrl==ctrl){
			int j;
			for(j=0;j<sizeof(exc[0].list)/sizeof(int);j++){
				if(exc[i].list[j]==0)
					break;
				else
					SendDlgItemMessage(hwnd,exc[i].list[j],BM_SETCHECK,BST_UNCHECKED,0);
			}
			break;
		}
	}
	return TRUE;
}
int load_icon(HWND hwnd)
{
	HICON hIcon = LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON));
    if(hIcon){
		SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
		SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		return TRUE;
	}
	return FALSE;
}
LRESULT CALLBACK MainDlg(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;

#ifdef _DEBUG
	if(FALSE)
//	if(message!=0x200&&message!=0x84&&message!=0x20&&message!=WM_ENTERIDLE)
	if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_DRAWITEM
		&&msg!=WM_CTLCOLORBTN&&msg!=WM_CTLCOLOREDIT)
	//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("*");
		print_msg(msg,lparam,wparam);
		tick=GetTickCount();
	}
#endif	
	switch(msg)
	{
	case WM_INITDIALOG:
		load_bitmaps(ghinstance);
		grippy=create_grippy(hwnd);
		BringWindowToTop(hwnd);
		SendDlgItemMessage(hwnd,IDC_COMBO_SEARCH,EM_LIMITTEXT,1024,0);
		SendDlgItemMessage(hwnd,IDC_COMBO_REPLACE,EM_LIMITTEXT,1024,0);
		SendDlgItemMessage(hwnd,IDC_COMBO_MASK,EM_LIMITTEXT,1024,0);
		SendDlgItemMessage(hwnd,IDC_COMBO_PATH,EM_LIMITTEXT,1024,0);
		SendDlgItemMessage(hwnd,IDC_DEPTH_LEVEL,EM_LIMITTEXT,3,0);
		set_fonts(hwnd);
		reset_line_width();
		get_ini_stuff(hwnd);
		load_window_size(hwnd,"MAIN_WINDOW");
		create_list_menu(hwnd);
		resize_main(hwnd);
		SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		load_icon(hwnd);
		break;
	case WM_HELP:
		show_main_help(hwnd,(HELPINFO *)lparam);
		return TRUE;
		break;
	case WM_SIZE:
		//modify_list();
		grippy_move(hwnd,grippy);
//		dump_main(hwnd);
		resize_main(hwnd);
	//	InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_MOUSEWHEEL:
		if(GetFocus()==GetDlgItem(hwnd,IDC_LIST1)){
			WPARAM _wparam=wparam&0xFFFF0000;
			if(wparam&MK_CONTROL)
				_wparam<<=1;
			if(wparam&MK_SHIFT)
				_wparam<<=2;
			if((wparam&MK_CONTROL) && (wparam&MK_SHIFT))
				_wparam=wparam<<4;
			SendMessage(GetDlgItem(hwnd,IDC_LIST1),msg,_wparam,lparam);
		}
		break;
	case WM_DRAWITEM:
		list_drawitem(hwnd,wparam,lparam);
		set_scroll_width(hwnd,IDC_LIST1);
		return TRUE;
		break;
	case WM_DROPFILES:
		process_drop(hwnd,(HANDLE)wparam,GetKeyState(VK_CONTROL)&0x8000,
			GetKeyState(VK_SHIFT)&0x8000);
		break;
	case WM_CONTEXTMENU:
		if(wparam==GetDlgItem(hwnd,IDC_LIST1)){
			POINT screen={0};
			GetCursorPos(&screen);
			set_listitem_mouse(hwnd);
			TrackPopupMenu(list_menu,TPM_LEFTALIGN,screen.x,screen.y,0,hwnd,NULL);
		}
		break;
	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case '0':case '1':case'2':case'3':case'4':case'5':
			{
				int index=CMD_OPENWITH+LOWORD(wparam)-'1';
			if(LOWORD(wparam)=='0')
				index=CMD_OPENASSOC;
			handle_context_open(hwnd,index);
			}
			break;
		case 'C':
			{
				int all=GetKeyState(VK_SHIFT)&0x8000;
				if(GetKeyState(VK_CONTROL)&0x8000)
					copy_items_clip(hwnd,all);
			}
			break;
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGEEX,0,0xFFFF);
			break;
		case 'S':
			if(GetKeyState(VK_CONTROL)&0x8000)
				PostMessage(GetDlgItem(hwnd,IDC_COMBO_SEARCH),WM_SETFOCUS,GetDlgItem(hwnd,IDC_LIST1),0);
			break;
		case 'R':
			if(GetKeyState(VK_CONTROL)&0x8000)
				PostMessage(GetDlgItem(hwnd,IDC_COMBO_REPLACE),WM_SETFOCUS,GetDlgItem(hwnd,IDC_LIST1),0);
			break;

		}
		return -1;
		break;
	case WM_CHARTOITEM:
		switch(wparam){
		case 'z':
			break;
		case 'x':
			break;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(GetDlgItem(hwnd,IDC_LIST1)==GetFocus()){
				view_context(hwnd);
				break;
			}
			start_search(hwnd,FALSE);
			InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
			break;
		case IDCANCEL:
			{
			HWND focus=GetFocus();
			if(GetDlgItem(hwnd,IDC_LIST1)==focus)
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,0,MAKELPARAM(0,0xFFFF));
			else if(GetDlgItem(hwnd,IDC_CHECK_DEPTH)==focus || GetDlgItem(hwnd,IDC_DEPTH_LEVEL)==focus)
				SetDlgItemText(hwnd,IDC_DEPTH_LEVEL,"0");
			}
		//	PostQuitMessage(0);
			break;
		default:
		case CMD_OPENWITH:
		case CMD_OPENASSOC:
			if(LOWORD(wparam)>=CMD_OPENASSOC && LOWORD(wparam)<CMD_END)
				handle_context_open(hwnd,LOWORD(wparam));
			break;
		case IDC_LIST1:
			switch(HIWORD(wparam)){
			case 1:
				SetFocus(GetDlgItem(hwnd,LOWORD(wparam)));
				break;
			case LBN_DBLCLK:
				view_context(hwnd);
				break;
			}
			break;
		case IDC_COMBO_SEARCH:
		case IDC_COMBO_REPLACE:
		case IDC_COMBO_MASK:
		case IDC_COMBO_PATH:
			if(HIWORD(wparam)==1){
				SetFocus(GetDlgItem(hwnd,LOWORD(wparam)));
				SendDlgItemMessage(hwnd,LOWORD(wparam),CB_SETEDITSEL,0,MAKELPARAM(0,-1));
			}
			break;
		case IDC_OPTIONS:
			do_options_dlg(hwnd);
			create_list_menu(hwnd);
			reset_line_width();
			set_fonts(hwnd);
			InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
			break;
		case IDC_REPLACE:
			start_search(hwnd,TRUE);
			InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
			SetFocus(GetDlgItem(hwnd,IDC_LIST1));
			break;
		case IDC_SEARCH:
			start_search(hwnd,FALSE);
			InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
			SetFocus(GetDlgItem(hwnd,IDC_LIST1));
			break;
		case IDC_SEARCH_OPTIONS:
			search_options(hwnd);
			break;
		case IDC_REPLACE_OPTIONS:
			replace_options(hwnd);
			break;
		case IDC_PATH_UPONE:
			path_up_level(hwnd,IDC_COMBO_PATH);
			break;
		case IDC_FILE_OPTIONS:
			handle_favorites(hwnd,IDC_FILE_OPTIONS);
			break;
		case IDC_PATH_OPTIONS:
			if(GetKeyState(VK_CONTROL)&0x8000){
				char path[MAX_PATH]={0};
				GetDlgItemText(hwnd,IDC_COMBO_PATH,path,sizeof(path));
				if(handle_browse_dialog(hwnd,path))
					SetDlgItemText(hwnd,IDC_COMBO_PATH,path);
			}
			else
				handle_favorites(hwnd,IDC_PATH_OPTIONS);
			break;
		case IDC_WHOLEWORD:
		case IDC_HEX:
		case IDC_UNICODE:
		case IDC_WILDCARD:
		case IDC_IGNOREWS:
			exclude_buttons(hwnd,LOWORD(wparam));
			break;
		case IDC_DEPTH_LEVEL:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				{
					char str[20]={0};
					int depth=MAXDWORD;
					GetDlgItemText(hwnd,IDC_DEPTH_LEVEL,str,sizeof(str));
					if(str[0]!=0)
						depth=atoi(str);
					if(depth>=999)
						depth=MAXDWORD;
					set_depth_limit(depth);
				}
				break;
			}
			break;
		case IDC_CHECK_DEPTH:
			{
			char str[20]={0};
			int flags=0;
			static last_depth=0;
			if(IsDlgButtonChecked(hwnd,LOWORD(wparam))!=BST_CHECKED){
				flags=SW_HIDE;
				set_depth_limit(MAXDWORD);
				GetDlgItemText(hwnd,IDC_DEPTH_LEVEL,str,sizeof(str));
				if(str[0]!=0)
					last_depth=atoi(str);
			}
			else{
				set_depth_limit(last_depth);
				_snprintf(str,sizeof(str),"%i",last_depth);
				SetWindowText(GetDlgItem(hwnd,IDC_DEPTH_LEVEL),str);
				flags=SW_SHOW;
			}
			ShowWindow(GetDlgItem(hwnd,IDC_DEPTH_LEVEL),flags);
			}
			break;
		case IDC_ONTOP:
			if(HIWORD(wparam)!=0){ //from accelerator
				int check;
				check=IsDlgButtonChecked(hwnd,LOWORD(wparam));
				if(check==BST_CHECKED)
					check=BST_UNCHECKED;
				else
					check=BST_CHECKED;
				CheckDlgButton(hwnd,LOWORD(wparam),check);
			}
			SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,LOWORD(wparam))?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			break;
		}
		break;
	case WM_QUERYENDSESSION:
		SetWindowLong(hwnd,DWL_MSGRESULT,TRUE); //ok to end session
		return TRUE;
	case WM_ENDSESSION:
		if(wparam){
			SetWindowText(hwnd,"Shutting down");
			save_window_size(hwnd,"MAIN_WINDOW");
			save_ini_stuff(hwnd);
			SetWindowLong(hwnd,DWL_MSGRESULT,0);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		save_window_size(hwnd,"MAIN_WINDOW");
		save_ini_stuff(hwnd);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine,int iCmdShow)
{
	MSG msg;
	HACCEL haccel;
	ghinstance=hInstance;

	init_ini_file();
	
	hwindow=CreateDialog(ghinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainDlg);
	if(!hwindow){
		
		MessageBox(NULL,"Could not create main dialog","ERROR",MB_ICONERROR | MB_OK);
		return 0;
	}

#ifdef _DEBUG
	open_console();
	move_console();
#endif
	ShowWindow(hwindow,iCmdShow);
	UpdateWindow(hwindow);

	if(szCmdLine!=0 && szCmdLine[0]!=0)
		cmdline=szCmdLine;

	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR));
	while(GetMessage(&msg,NULL,0,0))
	{
		{
			int _msg,lparam,wparam;
			_msg=msg.message;
			lparam=msg.lParam;
			wparam=msg.wParam;
			if(FALSE)
			if(_msg!=WM_MOUSEFIRST&&_msg!=WM_NCHITTEST&&_msg!=WM_SETCURSOR&&_msg!=WM_ENTERIDLE&&_msg!=WM_DRAWITEM
				&&_msg!=WM_CTLCOLORBTN&&_msg!=WM_CTLCOLOREDIT)
			//if(_msg!=WM_NCHITTEST&&_msg!=WM_SETCURSOR&&_msg!=WM_ENTERIDLE)
			{
				static DWORD tick=0;
				if((GetTickCount()-tick)>500)
					printf("--\n");
				printf("*");
				print_msg(_msg,lparam,wparam);
				tick=GetTickCount();
			}
		}
		if(haccel!=0)
			TranslateAccelerator(hwindow,haccel,&msg);
		if(!IsDialogMessage(hwindow,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}