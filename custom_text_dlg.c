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
extern char search_str[0x10000];
extern int strlen_search_str;
extern char replace_str[0x10000];
extern int strlen_replace_str;
enum{SEARCH,REPLACE};
static HWND hwnd_parent=0;
int custom_type=0;

int disable_options(HWND hwnd)
{
	int list[5]={IDC_UNICODE,IDC_WHOLEWORD,IDC_WILDCARD};
	int i;
	for(i=0;i<sizeof(list);i++){
		if(list[i]!=0)
			SendDlgItemMessage(hwnd,list[i],BM_SETCHECK,BST_UNCHECKED,0);
	}
	return TRUE;
}
int save_text(HWND hwnd)
{
	char *dest=0,*tmp=0;
	int extended;
	int size=sizeof(search_str)*2;
	if(custom_type==SEARCH)
		dest=search_str;
	else
		dest=replace_str;
	tmp=malloc(size);
	if(tmp!=0){
		memset(tmp,0,size);
		GetDlgItemText(hwnd,IDC_BIG_EDIT,tmp,size);
		extended=IsDlgButtonChecked(hwnd,IDC_EXTENDED)==BST_CHECKED;
		if(extended){
			int i,index,len,ctrl=0;
			len=strlen(tmp);
			index=0;
			for(i=0;i<len;i++){
				if(tmp[i]=='\\' && tmp[i+1]!=0){
					switch(tmp[i+1]){
					case '0':
						tmp[index++]=0;
						i++;
						continue;
					case 'r':
						tmp[index++]=0x0D;
						i++;
						continue;
					case 'n':
						tmp[index++]=0x0A;
						i++;
						continue;
					case 't':
						tmp[index++]=0x09;
						i++;
						continue;
					default:
						tmp[index++]=tmp[i];
						break;
					}
				}
				else
					tmp[index++]=tmp[i];
			}
			tmp[index]=0;
			if(custom_type==SEARCH){
				memcpy(search_str,tmp,index);
				strlen_search_str=index;
				ctrl=IDC_COMBO_SEARCH;
			}
			else{
				memcpy(replace_str,tmp,index);
				strlen_replace_str=index;
				ctrl=IDC_COMBO_REPLACE;
			}
			if(index>0){
				SetDlgItemText(hwnd_parent,ctrl,"Binary mode -->>");
				if(custom_type==SEARCH)
					disable_options(hwnd_parent);
			}

		}
		else{
			int len,ctrl=0;
			len=strlen(tmp);
			if(len>sizeof(search_str))
				len=sizeof(search_str);
			if(custom_type==SEARCH){
				strncpy(search_str,tmp,len);
				strlen_search_str=len;
				ctrl=IDC_COMBO_SEARCH;
			}
			else{
				strncpy(replace_str,tmp,len);
				strlen_replace_str=len;
				ctrl=IDC_COMBO_REPLACE;
			}
			if(len>0){
				SetDlgItemText(hwnd_parent,ctrl,"Binary mode -->>");
				if(custom_type==SEARCH)
					disable_options(hwnd_parent);
			}
		}
		free(tmp);
	}
	return TRUE;
}
int load_text(HWND hwnd)
{
	char *str=0,*text=0;
	int i,index,len,ctrl,extended;
	int size=sizeof(search_str)*2;
	str=malloc(size);
	if(str==0)
		return FALSE;
	if(custom_type==SEARCH){
		ctrl=IDC_COMBO_SEARCH;
		text=search_str;
		len=strlen_search_str;
	}
	else{
		ctrl=IDC_COMBO_REPLACE;
		text=replace_str;
		len=strlen_replace_str;
	}
	str[0]=0;
	GetDlgItemText(hwnd_parent,ctrl,str,size);
	if(strcmp(str,"Binary mode -->>")!=0){
		SetDlgItemText(hwnd,IDC_BIG_EDIT,str);
		free(str);
		return TRUE;
	}
	if(len>size)
		len=size;
	index=0;extended=FALSE;
	for(i=0;i<len;i++){
		if(index<size-2){
			switch(text[i]){
			case 0:
				str[index++]='\\';
				str[index++]='0';
				extended=TRUE;
				continue;
			default:
				str[index++]=text[i];
				break;
			}
		}
		else if(index<size-1)
			str[index++]=text[i];
	}
	if(extended)
		CheckDlgButton(hwnd,IDC_EXTENDED,BST_CHECKED);
	str[index]=0;
	SetDlgItemText(hwnd,IDC_BIG_EDIT,str);
	free(str);
	return TRUE;
}
LRESULT CALLBACK custom_text_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg)
	{
	case WM_INITDIALOG:
		grippy=create_grippy(hwnd);
		SetFocus(GetDlgItem(hwnd,IDC_BIG_EDIT));
		load_text(hwnd);
		SendDlgItemMessage(hwnd,IDC_BIG_EDIT,EM_SETSEL,0,-1);
		if(custom_type==REPLACE)
			SetWindowText(hwnd,"Custom Replace");
		init_cust_text_win_anchor(hwnd);
		restore_cust_win_rel_pos(hwnd);
		return 0;
	case WM_DESTROY:
		save_cust_win_rel_pos(hwnd);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_cust_text_win(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(GetWindowTextLength(GetDlgItem(hwnd,IDC_BIG_EDIT))>0){
				save_text(hwnd);
			}
		case IDCANCEL:
			EndDialog(hwnd,0);
		}
		break;
	}
	return 0;
}


int search_options(HWND hwnd)
{
	hwnd_parent=hwnd;
	custom_type=SEARCH;
	return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_CUSTOM_SEARCH),hwnd,custom_text_proc);
}

int replace_options(HWND hwnd)
{
	hwnd_parent=hwnd;
	custom_type=REPLACE;
	return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_CUSTOM_SEARCH),hwnd,custom_text_proc);
}