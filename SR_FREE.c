#define WINVER 0x500
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <fcntl.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include "resource.h"

HWND		ghwindow;
HINSTANCE	ghinstance;
int _fseeki64(FILE *stream,__int64 offset,int origin);
__int64 _ftelli64(FILE *stream);
char *cmdline=0;
static HMENU list_menu=0;
static HWND gresult_list=0;

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

int show_main_help(HWND hwnd,HELPINFO *hi)
{
	static int help_active=FALSE;
	if(!help_active){
		help_active=TRUE;
		switch(hi->iCtrlId){
		case IDC_COMBO_SEARCH:
			if(is_button_checked(hwnd,IDC_REGEX)
				&& (!(GetKeyState(VK_CONTROL)&0x8000)) ){
				const char *msg=
					"TRex implements the following expressions\r\n"
					"\r\n"
					"\\	Quote the next metacharacter\r\n"
					"^	Match the beginning of the string\r\n"
					".	Match any character\r\n"
					"$	Match the end of the string\r\n"
					"|	Alternation\r\n"
					"()	Grouping (creates a capture)\r\n"
					"[]	Character class  \r\n"
					"\r\n"
					"==GREEDY CLOSURES==\r\n"
					"*	Match 0 or more times\r\n"
					"+	Match 1 or more times\r\n"
					"?	Match 1 or 0 times\r\n"
					"{n}	Match exactly n times\r\n"
					"{n,}	Match at least n times\r\n"
					"{n,m}	Match at least n but not more than m times  \r\n"
					"\r\n"
					"==ESCAPE CHARACTERS==\r\n"
					"\\t	tab                   (HT, TAB)\r\n"
					"\\n	newline               (LF, NL)\r\n"
					"\\r	return                (CR)\r\n"
					"\\f	form feed             (FF)\r\n"
					"\r\n"
					"==PREDEFINED CLASSES==\r\n"
					"\\l	lowercase next char\r\n"
					"\\u	uppercase next char\r\n"
					"\\a	letters\r\n"
					"\\A	non letters\r\n"
					"\\w	alphanimeric [0-9a-zA-Z]\r\n"
					"\\W	non alphanimeric\r\n"
					"\\s	space\r\n"
					"\\S	non space\r\n"
					"\\d	digits\r\n"
					"\\D	non nondigits\r\n"
					"\\x	exadecimal digits\r\n"
					"\\X	non exadecimal digits\r\n"
					"\\c	control charactrs\r\n"
					"\\C	non control charactrs\r\n"
					"\\p	punctation\r\n"
					"\\P	non punctation\r\n"
					"\\b	word boundary\r\n"
					"\\B	non word boundary";
				MessageBox(hwnd,msg,"REGEXHELP",MB_OK);
				break;
			}
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
					"ctrl+p=set path focus\r\n"
					"ctrl+backspace=move up dir\r\n\r\n"
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
int check_ext_favs(char *mask,int mask_size,char *ext)
{
	int result=FALSE;
	char *tmpstr=0;
	int tmpsize=mask_size;
	int max_favs;
	int i;
	if(mask_size<=2 || mask==0 || ext==0)
		return result;
	if(ext[0]==0)
		return result;
	tmpstr=malloc(tmpsize);
	if(tmpstr==0)
		return result;
	max_favs=get_max_favs();

	for(i=0;i<max_favs;i++){
		char key[10]={0};
		_snprintf(key,sizeof(key),"item%i",i);
		tmpstr[0]=0;
		get_ini_str("FILE_MASK_FAVS",key,tmpstr,tmpsize);
		if(tmpstr[0]=='>'){
			char *orig;
			int origsize=tmpsize;
			orig=malloc(origsize);
			if(orig!=0){
				char *token;
				int j=0;
				strncpy(orig,tmpstr+1,origsize);
				orig[origsize-1]=0;
				tmpstr[tmpsize-1]=0;
				token=strtok(tmpstr+1,";"); //skip >
				while(token!=0){
					if(wild_card_match(token,ext)){
						strncpy(mask,orig,mask_size);
						mask[mask_size-1]=0;
						result=TRUE;
						break;
					}
					token=strtok(NULL,";");
					j++;
					if(j>100)
						break;
				}
				free(orig);
			}
		}
		if(result)
			break;
	}
	if(tmpstr!=0)
		free(tmpstr);
	return result;
}
int process_drop(HWND hwnd,HANDLE hdrop,int ctrl,int shift,int alt)
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
			if(alt)
				; //dont alter mask
			else if(shift && (!ctrl)){
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
			else if((!ctrl) && (!shift)){
				if(check_ext_favs(mask,sizeof(mask),ext))
					SetWindowText(GetDlgItem(hwnd,IDC_COMBO_MASK),mask);
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
	if(ctrl && (!shift)){ //only search this file
		SendDlgItemMessage(hwnd,IDC_SUBDIRS,BM_SETCHECK,BST_UNCHECKED,0);
		ShowWindow(GetDlgItem(hwnd,IDC_CHECK_DEPTH),SW_HIDE);
		ShowWindow(GetDlgItem(hwnd,IDC_DEPTH_LEVEL),SW_HIDE);
	}
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
int load_combo_box(HWND hwnd,int ctrl,const char *keystart)
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
struct CTRL_SETTING{
	int ctrl;
	const char *name;
};
static struct CTRL_SETTING combo_boxs[]={
	{IDC_COMBO_SEARCH,"search"},
	{IDC_COMBO_MASK,"mask"},
	{IDC_COMBO_PATH,"path"}
};
static struct CTRL_SETTING pushbuttons[]={
	{IDC_SUBDIRS,"search_subdirs"},
	{IDC_UNICODE,"unicode"},
	{IDC_CASE,"case"},
	{IDC_WHOLEWORD,"wholeword"},
	{IDC_HEX,"hex"},
	{IDC_WILDCARD,"wildcard"},
	{IDC_REGEX,"regex"},
	{IDC_ONTOP,"on_top"}
};

int get_ini_stuff(HWND hwnd)
{
	int i;
	for(i=0;i<sizeof(combo_boxs)/sizeof(struct CTRL_SETTING);i++){
		load_combo_box(hwnd,combo_boxs[i].ctrl,combo_boxs[i].name);
	}
	for(i=0;i<sizeof(pushbuttons)/sizeof(struct CTRL_SETTING);i++){
		int val=FALSE;
		get_ini_value("BUTTON_SETTINGS",pushbuttons[i].name,&val);
		if(val)
			SendDlgItemMessage(hwnd,pushbuttons[i].ctrl,BM_SETCHECK,BST_CHECKED,0);
	}
	return TRUE;
}
int save_combo_box(HWND hwnd,int ctrl,const char *keystart)
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
	int i;
	for(i=0;i<sizeof(combo_boxs)/sizeof(struct CTRL_SETTING);i++){
		save_combo_box(hwnd,combo_boxs[i].ctrl,combo_boxs[i].name);
	}
	for(i=0;i<sizeof(pushbuttons)/sizeof(struct CTRL_SETTING);i++){
		write_ini_value("BUTTON_SETTINGS",pushbuttons[i].name,is_button_checked(hwnd,pushbuttons[i].ctrl));
	}
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
				if(str[0]!=0){
					char path[_MAX_PATH];
					strncpy(path,fname,sizeof(path));
					path[sizeof(path)-1]=0;
					find_valid_path(path);
					shell_execute(hwnd,str,path);
				}
				free(str);
			}
			free(buf);
		}
	}
	return TRUE;
}
int get_nearest_filename(HWND hwnd,WCHAR *fname,int flen)
{
	int index,fitem=-1;
	index=SendDlgItemMessageW(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(index>=0){
		int i;
		for(i=index;i>=0;i--){
			int len;
			WCHAR *buf=0;
			len=SendDlgItemMessageW(hwnd,IDC_LIST1,LB_GETTEXTLEN,i,0);
			if(len>0){
				buf=malloc((len+1)*sizeof(WCHAR));
				if(buf!=0){
					buf[0]=0;
					SendDlgItemMessageW(hwnd,IDC_LIST1,LB_GETTEXT,i,buf);
					if(wcsnicmp(buf,L"file",sizeof(L"file")/sizeof(WCHAR)-1)==0){
						wcsncpy(fname,buf+sizeof(L"file ")/sizeof(WCHAR)-1,flen);
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
	WCHAR fname[SR_MAX_PATH];
	int index;
	if(cmd==CMD_SEARCH_ALL_FILE){
		SetDlgItemText(hwnd,IDC_COMBO_MASK,"*.*");
		return TRUE;
	}
	fname[0]=0;
	get_nearest_filename(hwnd,fname,sizeof(fname)/sizeof(WCHAR));
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCARETINDEX,0,0);
	if(fname[0]!=0 && index>=0){
		switch(cmd){
		default:
			{
				char tmp[MAX_PATH];
				wcstombs(tmp,fname,sizeof(tmp));
				tmp[sizeof(tmp)-1]=0;
				open_with_cmd(hwnd,tmp,index,cmd);
			}
			break;
		case CMD_SEARCH_THIS_FOLDER:
			{
				WCHAR path[SR_MAX_PATH];
				path[0]=0;
				splitpath_long(fname,path,sizeof(path)/sizeof(WCHAR),0,0);
				if(path[0]!=0)
					SetDlgItemTextW(hwnd,IDC_COMBO_PATH,path);
			}
			break;
		case CMD_SEARCH_THIS_FILE_TYPE:
			{
				WCHAR ext[SR_MAX_PATH];
				ext[0]=0;
				splitpath_long(fname,0,0,ext,sizeof(ext)/sizeof(WCHAR));
				if(ext[0]!=0){
					int i,index=0,start=FALSE;
					WCHAR mask[MAX_PATH];
					mask[index++]=L'*';
					for(i=0;i<SR_MAX_PATH;i++){
						WCHAR a=ext[i];
						if(a==L'.')
							start=TRUE;
						if(start){
							if(index>=(sizeof(mask)/sizeof(WCHAR)-1))
								break;
							mask[index++]=a;
						}
					}
					if(index<sizeof(mask)/sizeof(WCHAR))
						mask[index]=0;
					if(start)
						SetDlgItemTextW(hwnd,IDC_COMBO_MASK,mask);
				}
			}
			break;
		case CMD_SEARCH_THIS_FILE:
			{
				WCHAR name[SR_MAX_PATH],path[SR_MAX_PATH];
				name[0]=0;
				splitpath_long(fname,path,sizeof(path)/sizeof(WCHAR),name,sizeof(name)/sizeof(WCHAR));
				path[sizeof(path)/sizeof(WCHAR)-1]=0;
				name[sizeof(name)/sizeof(WCHAR)-1]=0;
				if(name[0]!=0 && path[0]!=0){
					SetDlgItemTextW(hwnd,IDC_COMBO_MASK,name);
					SetDlgItemTextW(hwnd,IDC_COMBO_PATH,path);
				}

			}
			break;
		case CMD_OPENASSOC:
			ShellExecuteW(hwnd,L"open",fname,NULL,NULL,SW_SHOWNORMAL);
			break;

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
int clamp_window_size(int *x,int *y,int *width,int *height,RECT *monitor)
{
	int mwidth,mheight;
	mwidth=monitor->right-monitor->left;
	mheight=monitor->bottom-monitor->top;
	if(mwidth<=0)
		return FALSE;
	if(mheight<=0)
		return FALSE;
	if(*x<monitor->left)
		*x=monitor->left;
	if(*width>mwidth)
		*width=mwidth;
	if(*x+*width>monitor->right)
		*x=monitor->right-*width;
	if(*y<monitor->top)
		*y=monitor->top;
	if(*height>mheight)
		*height=mheight;
	if(*y+*height>monitor->bottom)
		*y=monitor->bottom-*height;
	return TRUE;
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
			if(width<50 || height<50){
				flags|=SWP_NOSIZE;
			}
			if(!clamp_window_size(&x,&y,&width,&height,&rect))
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
		int list[6];
	}EXCLUDE;
	EXCLUDE exc[]={
		{IDC_HEX,{IDC_UNICODE,IDC_WHOLEWORD,IDC_WILDCARD,IDC_REGEX,0}},
		{IDC_UNICODE,{IDC_HEX,IDC_WHOLEWORD,IDC_WILDCARD,IDC_REGEX,0}},
		{IDC_WHOLEWORD,{IDC_HEX,IDC_UNICODE,IDC_WILDCARD,IDC_REGEX,0}},
		{IDC_WILDCARD,{IDC_HEX,IDC_UNICODE,IDC_WHOLEWORD,IDC_REGEX,0}},
		{IDC_REGEX,   {IDC_HEX,IDC_UNICODE,IDC_WHOLEWORD,IDC_WILDCARD,IDC_CASE}},
		{IDC_CASE ,   {0}},
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
int process_custom_hlist(MSG *msg,HWND hlist)
{
	static int rmb_pressed=FALSE;
	if(hlist==0)
		return FALSE;
	if(rmb_pressed && msg->message==WM_RBUTTONUP){
		rmb_pressed=FALSE;
		return TRUE;
	}
	if(msg->message==WM_MOUSEWHEEL){
		int dir;
		short delta=(short)HIWORD(msg->wParam);
		if(msg->wParam&MK_RBUTTON){
			if(delta<0)
				dir=SB_PAGEDOWN;
			else
				dir=SB_PAGEUP;
			SendMessage(hlist,WM_VSCROLL,dir,0);
			rmb_pressed=TRUE;
			return TRUE;
		}
		else if(msg->wParam&MK_SHIFT){
			if(delta<0)
				dir=SB_PAGEDOWN;
			else
				dir=SB_PAGEUP;
			SendMessage(hlist,WM_VSCROLL,dir,0);
			return TRUE;
		}
		else if(msg->wParam&MK_CONTROL){
			if(delta<0)
				dir=SB_LINEDOWN;
			else
				dir=SB_LINEUP;
			SendMessage(hlist,WM_VSCROLL,dir,0);
			return TRUE;
		}
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
		init_main_win_anchor(hwnd);
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
		SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
		SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_SUBDIRS,BN_CLICKED),0);
		load_icon(hwnd);
		register_drag_drop(hwnd);
		gresult_list=GetDlgItem(hwnd,IDC_LIST1);
		break;
	case WM_HELP:
		show_main_help(hwnd,(HELPINFO *)lparam);
		return TRUE;
		break;
	case WM_MOVING:
		if(GetKeyState(VK_CONTROL)&0x8000)
			snap_window(hwnd,lparam);
		break;
	case WM_SIZING:
		if(GetKeyState(VK_CONTROL)&0x8000)
			snap_sizing(hwnd,lparam,wparam);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_main_win(hwnd);
		break;
	case WM_DRAWITEM:
		list_drawitem(hwnd,wparam,lparam);
		set_scroll_width(hwnd,IDC_LIST1);
		return TRUE;
		break;
	case WM_DROPFILES:
		process_drop(hwnd,(HANDLE)wparam,GetKeyState(VK_CONTROL)&0x8000,
			GetKeyState(VK_SHIFT)&0x8000,GetKeyState(VK_MENU)&0x8000);
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
		{
			int ctrl,shift,key;
			ctrl=GetKeyState(VK_CONTROL)&0x8000;
			shift=GetKeyState(VK_SHIFT)&0x8000;
			key=LOWORD(wparam);
			switch(key){
			case '0':case '1':case'2':case'3':case'4':case'5':case'6':case'7':case'8':case'9':
				{
					int index=CMD_OPENWITH+LOWORD(wparam)-'1';
				if(LOWORD(wparam)=='0')
					index=CMD_OPENASSOC;
				handle_context_open(hwnd,index);
				}
				break;
			case 'C':
				{
					if(ctrl)
						copy_items_clip(hwnd,shift);
				}
				break;
			case 'A':
				if(ctrl)
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGEEX,0,0xFFFF);
				break;
			}
		}
		return -1;
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(GetDlgItem(hwnd,IDC_LIST1)==GetFocus()){
				view_context(hwnd);
				break;
			}
			else{
				int result;
				result=start_search(hwnd,FALSE);
				InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
				if(result)
					SetFocus(GetDlgItem(hwnd,IDC_LIST1));
			}
			break;
		case IDCANCEL:
			{
			HWND focus=GetFocus();
			if(GetDlgItem(hwnd,IDC_LIST1)==focus)
				SendDlgItemMessage(hwnd,IDC_LIST1,LB_SELITEMRANGE,0,MAKELPARAM(0,0xFFFF));
			else if(GetDlgItem(hwnd,IDC_CHECK_DEPTH)==focus || GetDlgItem(hwnd,IDC_DEPTH_LEVEL)==focus)
				SetDlgItemText(hwnd,IDC_DEPTH_LEVEL,"0");
			cancel_search();
			}
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
				if(lparam==0)
					SetFocus(GetDlgItem(hwnd,LOWORD(wparam)));
				break;
			case LBN_DBLCLK:
				if((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000) || (GetKeyState(VK_MENU)&0x8000))
					open_url_listview(hwnd);
				else
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
			{
				int result;
				result=start_search(hwnd,FALSE);
				InvalidateRect(GetDlgItem(hwnd,IDC_LIST1),NULL,TRUE);
				if(result)
					SetFocus(GetDlgItem(hwnd,IDC_LIST1));
			}
			break;
		case IDC_SEARCH_OPTIONS:
			search_options(hwnd);
			break;
		case IDC_REPLACE_OPTIONS:
			replace_options(hwnd);
			break;
		case IDC_PATH_UPONE:
			if((1==HIWORD(wparam)) && GetFocus()!=GetDlgItem(hwnd,IDC_LIST1))
				break;
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
		case IDC_SUBDIRS:
			if(IsDlgButtonChecked(hwnd,IDC_SUBDIRS)){
				ShowWindow(GetDlgItem(hwnd,IDC_CHECK_DEPTH),SW_SHOW);
				if(IsDlgButtonChecked(hwnd,IDC_CHECK_DEPTH))
					ShowWindow(GetDlgItem(hwnd,IDC_DEPTH_LEVEL),SW_SHOW);
			}
			else{
				ShowWindow(GetDlgItem(hwnd,IDC_CHECK_DEPTH),SW_HIDE);
				ShowWindow(GetDlgItem(hwnd,IDC_DEPTH_LEVEL),SW_HIDE);
			}
			break;
		case IDC_WHOLEWORD:
		case IDC_HEX:
		case IDC_UNICODE:
		case IDC_WILDCARD:
		case IDC_REGEX:
		case IDC_CASE:
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
			if(!(GetKeyState(VK_SHIFT)&0x8000)){
				save_window_size(hwnd,"MAIN_WINDOW");
				save_ini_stuff(hwnd);
			}
			SetWindowLong(hwnd,DWL_MSGRESULT,0);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		if(!(GetKeyState(VK_SHIFT)&0x8000)){
			save_window_size(hwnd,"MAIN_WINDOW");
			save_ini_stuff(hwnd);
		}
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

	OleInitialize(0);
	init_ini_file();

#ifdef _DEBUG
	open_console();
	move_console();
#endif

	ghwindow=CreateDialog(ghinstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainDlg);
	if(!ghwindow){

		MessageBox(NULL,"Could not create main dialog","ERROR",MB_ICONERROR | MB_OK);
		return 0;
	}

	ShowWindow(ghwindow,iCmdShow);
	UpdateWindow(ghwindow);

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
			if(gresult_list!=0 && msg.hwnd==gresult_list){
				if(process_custom_hlist(&msg,gresult_list))
					continue;
			}
		}
		if(haccel!=0)
			TranslateAccelerator(ghwindow,haccel,&msg);
		if(!IsDialogMessage(ghwindow,&msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}
