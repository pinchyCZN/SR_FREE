#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <fcntl.h>
#include <shlwapi.h>
#include <Shlobj.h>
#include "resource.h"

extern HINSTANCE	ghinstance;
int fav_type=0;
HWND hwnd_parent=0;
#define MAX_FAVS 50
int load_favs(HWND hwnd,int ctrl)
{
	int i;
	char key[40],str[1024],*section=0;
	if(fav_type==IDC_FILE_OPTIONS)
		section="FILE_MASK_FAVS";
	else
		section="PATH_FAVS";
	for(i=0;i<MAX_FAVS;i++){
		str[0]=0;
		_snprintf(key,sizeof(key),"item%i",i);
		get_ini_str(section,key,str,sizeof(str));
		if(str[0]!=0)
			SendDlgItemMessage(hwnd,ctrl,LB_ADDSTRING,0,str);
	}
	return TRUE;
}
int save_favs(HWND hwnd,int ctrl)
{
	int i,fav,count;
	char key[40],str[1024],*section=0;
	count=SendDlgItemMessage(hwnd,ctrl,LB_GETCOUNT,0,0);
	if(fav_type==IDC_FILE_OPTIONS)
		section="FILE_MASK_FAVS";
	else
		section="PATH_FAVS";
	fav=0;
	for(i=0;i<count;i++){
		str[0]=0;
		if(SendDlgItemMessage(hwnd,ctrl,LB_GETTEXT,i,str)>0){
			if(str[0]!=0){
				_snprintf(key,sizeof(key),"item%i",fav);
				write_ini_str(section,key,str);
				fav++;
			}
		}
	}
	for( ;i<MAX_FAVS;i++){
		_snprintf(key,sizeof(key),"item%i",fav);
		delete_ini_key(section,key);
		fav++;
	}
	return TRUE;
}
int load_favs_size(HWND hwnd)
{
	int width=0,height=0;
	get_ini_value("FAVS_WINDOW","width",&width);
	get_ini_value("FAVS_WINDOW","height",&height);
	if(width>0 && height>0)
		SetWindowPos(hwnd,NULL,NULL,NULL,width,height,SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
	return 0;
}
int save_favs_size(HWND hwnd)
{
	RECT rect={0};
	int x,y;
	GetWindowRect(hwnd,&rect);
	x=rect.right-rect.left;
	y=rect.bottom-rect.top;
	if(x<100)x=100;
	if(y<100)y=100;
	write_ini_value("FAVS_WINDOW","width",x);
	write_ini_value("FAVS_WINDOW","height",y);
	return TRUE;
}
int load_parent_text(HWND hwnd,int ctrl,int parent_ctrl)
{
	char str[1024]={0};
	GetDlgItemText(hwnd_parent,parent_ctrl,str,sizeof(str));
	if(str[0]!=0)
		SetDlgItemText(hwnd,IDC_FAV_EDIT,str);
	return TRUE;
}
int does_string_exist(HWND hwnd,int ctrl,char *str)
{
	int i,count,found=FALSE;
	char tmp[1024];
	count=SendDlgItemMessage(hwnd,ctrl,LB_GETCOUNT,0,0);
	if(count<=0)
		return found;
	for(i=0;i<count;i++){
		tmp[0]=0;
		SendDlgItemMessage(hwnd,ctrl,LB_GETTEXT,i,tmp);
		if(tmp[0]!=0){
			int j,len,diff=FALSE;
			len=strlen(tmp);
			if(len>sizeof(tmp))len=sizeof(tmp);
			if(len!=strlen(str))
				continue;
			for(j=0;j<len;j++){
				if(tolower(str[j])!=tolower(tmp[j])){
					diff=TRUE;
					break;
				}
			}
			if(!diff)
				found=TRUE;
		}
	}
	return found;
}
int get_parent_combo()
{
	if(fav_type==IDC_FILE_OPTIONS)
		return IDC_COMBO_MASK;
	if(fav_type==IDC_PATH_OPTIONS)
		return IDC_COMBO_PATH;
	return 0;
}
int delete_selected_item(HWND hwnd)
{
	int index;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
	if(index!=LB_ERR){
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_DELETESTRING,index,0);
		save_favs(hwnd,IDC_LIST1);
	}
	return 0;
}
int select_and_edit(HWND hwnd)
{
	int index;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
	if(index!=LB_ERR){
		char str[1024]={0};
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,index,str);
		if(str[0]!=0){
			SetDlgItemText(hwnd,IDC_FAV_EDIT,str);
		}
	}
	return 0;
}
int select_and_close(HWND hwnd)
{
	char str[1024]={0};
	int index;
	index=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
	if(index!=LB_ERR){
		SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,index,str);
	}
	else{
		GetDlgItemText(hwnd,IDC_FAV_EDIT,str,sizeof(str));
	}
	str[sizeof(str)-1]=0;
	str_trim_right(str);
	if(str[0]!=0){
		char *s=str;
		if(s[0]=='>')
			s++;
		SetDlgItemText(hwnd_parent,get_parent_combo(),s);
		save_favs_size(hwnd);
		EndDialog(hwnd,0);
	}
	return 0;
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
	SendDlgItemMessage(hwnd,IDC_LIST1,WM_SETFONT,GetStockObject(font),0);
	SendDlgItemMessage(hwnd,IDC_FAV_EDIT,WM_SETFONT,GetStockObject(font),0);
	return TRUE;
}
static int process_drop(HWND hwnd,HANDLE hdrop,int ctrl,int shift)
{
	int i,count;
	char str[MAX_PATH];
	count=DragQueryFile(hdrop,-1,NULL,0);
	for(i=0;i<count;i++){
		str[0]=0;
		DragQueryFile(hdrop,i,str,sizeof(str));
		if(is_path_directory(str)){
			if(fav_type==IDC_FILE_OPTIONS)
				continue;
			else{
				SetDlgItemText(hwnd,IDC_FAV_EDIT,str);
				break;
			}
		}
		else{
			char drive[_MAX_DRIVE],fpath[_MAX_PATH],fname[_MAX_PATH],dir[_MAX_PATH],ext[_MAX_EXT];
			_splitpath(str,drive,dir,fname,ext);
			if(fav_type==IDC_FILE_OPTIONS){
				_snprintf(str,sizeof(str),"*%s",ext);
				SetDlgItemText(hwnd,IDC_FAV_EDIT,str);
				break;
			}
			else{
				_snprintf(fpath,sizeof(fpath),"%s%s",drive,dir);
				SetDlgItemText(hwnd,IDC_FAV_EDIT,fpath);
				break;
			}
		}
	}
	DragFinish(hdrop);
	return TRUE;
}
LRESULT CALLBACK favorites_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
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
		grippy=create_grippy(hwnd);
		if(fav_type==IDC_FILE_OPTIONS){
			ShowWindow(GetDlgItem(hwnd,IDC_BROWSE_DIR),SW_HIDE);
			load_favs(hwnd,IDC_LIST1);
			load_parent_text(hwnd,IDC_FAV_EDIT,IDC_COMBO_MASK);
			SetWindowText(hwnd,"File Mask Favs");
		}
		else{
			load_favs(hwnd,IDC_LIST1);
			load_parent_text(hwnd,IDC_FAV_EDIT,IDC_COMBO_PATH);
			SetWindowText(hwnd,"Path Favs");
		}
		SendDlgItemMessage(hwnd,IDC_FAV_EDIT,EM_SETLIMITTEXT,1024-1,0);
		SetFocus(GetDlgItem(hwnd,IDC_FAV_EDIT));
		SendDlgItemMessage(hwnd,IDC_FAV_EDIT,EM_SETSEL,0,-1);
		set_fonts(hwnd);
		load_favs_size(hwnd);
		resize_favs(hwnd);
		return 0;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		InvalidateRect(hwnd,NULL,TRUE);
		resize_favs(hwnd);
		break;
	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case VK_DELETE:
			delete_selected_item(hwnd);
			break;
		}
		return -1;
		break;
	case WM_DROPFILES:
		process_drop(hwnd,(HANDLE)wparam,GetKeyState(VK_CONTROL)&0x8000,
			GetKeyState(VK_SHIFT)&0x8000);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_LIST1:
			switch(HIWORD(wparam)){
			case LBN_SELCHANGE:
				select_and_edit(hwnd);
				break;
			case LBN_DBLCLK:
				select_and_close(hwnd);
				break;
			}
			break;
		case IDC_BROWSE_DIR:
			{
			char path[MAX_PATH]={0};
			GetDlgItemText(hwnd,IDC_FAV_EDIT,path,sizeof(path));
			if(handle_browse_dialog(hwnd,path)){
				SetDlgItemText(hwnd_parent,IDC_COMBO_PATH,path);
				EndDialog(hwnd,0);
			}
			}
			break;
		case IDC_DELETE:
			delete_selected_item(hwnd);
			break;
		case IDC_SELECT:
			select_and_close(hwnd);
			break;
		case IDOK:
			if(GetFocus()==GetDlgItem(hwnd,IDC_LIST1)){
				select_and_close(hwnd);
				break;
			}
		case IDC_ADD:
			{
				char str[1024]={0};
				GetDlgItemText(hwnd,IDC_FAV_EDIT,str,sizeof(str));
				str_trim_right(str);
				if((str[0]!=0) && (!does_string_exist(hwnd,IDC_LIST1,str))){
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_ADDSTRING,0,str);
					save_favs(hwnd,IDC_LIST1);
				}
			}
			break;
		case IDCANCEL:
			save_favs_size(hwnd);
			EndDialog(hwnd,0);
		}
		break;
	}
	return 0;	
}
int handle_favorites(HWND hwnd,int ctrl)
{
	hwnd_parent=hwnd;
	fav_type=ctrl;
	return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_FAVORITES),hwnd,favorites_proc);
}