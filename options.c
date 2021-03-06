#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include "resource.h"

int max_items_combobox=10;
int max_open_with=10;

struct FONT_NAME{
	int font_num;
	char *font_name;
};
struct FONT_NAME font_names[7]={
	{OEM_FIXED_FONT,"OEM_FIXED_FONT"},
	{ANSI_FIXED_FONT,"ANSI_FIXED_FONT"},
	{ANSI_VAR_FONT,"ANSI_VAR_FONT"},
	{SYSTEM_FONT,"SYSTEM_FONT"},
	{DEVICE_DEFAULT_FONT,"DEVICE_DEFAULT_FONT"},
	{SYSTEM_FIXED_FONT,"SYSTEM_FIXED_FONT"},
	{DEFAULT_GUI_FONT,"DEFAULT_GUI_FONT"}
};

int get_max_open_with()
{
	return max_open_with;
}
int get_max_items_combobox()
{ return max_items_combobox; }

int add_fonts(HWND hwnd,int ctrl)
{
	int i;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++)
		SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,font_names[i].font_name);
	return TRUE;
}
int get_dropdown_name(int ctrl,char *name,int len){
	switch(ctrl){
	default:
	case IDC_COMBO_FONT:
		_snprintf(name,len,"%s","combo_font");
		name[len-1]=0;
		break;
	case IDC_LISTBOX_FONT:
		_snprintf(name,len,"%s","listbox_font");
		name[len-1]=0;
		break;
	}
	return TRUE;
}
int int_to_fontname(int font,char *name,int len)
{
	int i,result=FALSE;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++){
		if(font==font_names[i].font_num){
			_snprintf(name,len,font_names[i].font_name);
			result=TRUE;
			break;
		}
	}
	if(!result){
		_snprintf(name,len,"SYSTEM_FONT");
		result=TRUE;
	}
	name[len-1]=0;
	return result;
}
int fontname_to_int(char *name)
{
	int i;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++){
		if(stricmp(name,font_names[i].font_name)==0){
			return font_names[i].font_num;
		}
	}
	return DEFAULT_GUI_FONT;
}
int get_current_font(HWND hwnd,int ctrl)
{
	char tmp[80];
	char key[80];
	int index;
	key[0]=0;
	get_dropdown_name(ctrl,key,sizeof(key));
	tmp[0]=0;
	get_ini_str("OPTIONS",key,tmp,sizeof(tmp));
	index=SendDlgItemMessage(hwnd,ctrl,CB_FINDSTRINGEXACT,-1,tmp);
	if(index<0){
		int_to_fontname(DEFAULT_GUI_FONT,tmp,sizeof(tmp));
		index=SendDlgItemMessage(hwnd,ctrl,CB_FINDSTRINGEXACT,-1,tmp);
	}
	SendDlgItemMessage(hwnd,ctrl,CB_SETCURSEL,index,0);
	return TRUE;
}
int populate_caption_open(HWND hwnd){
	char str[MAX_PATH*2];
	char key[80];
	int index;
	index=SendDlgItemMessage(hwnd,IDC_SELECT_OPEN,CB_GETCURSEL,0,0);
	if(index<0 || index>max_open_with)
		index=0;
	index++;
	_snprintf(key,sizeof(key),"open%i",index);
	str[0]=0;
	get_ini_str("OPTIONS",key,str,sizeof(str));
	pipe_to_quote(str);
	SetDlgItemText(hwnd,IDC_OPEN1,str);
	_snprintf(key,sizeof(key),"caption%i",index);
	str[0]=0;
	get_ini_str("OPTIONS",key,str,sizeof(str));
	SetDlgItemText(hwnd,IDC_CAPTION,str);
	return TRUE;
}
int populate_options(HWND hwnd)
{
	int i;
	char tmp[80];
	SendDlgItemMessage(hwnd,IDC_SELECT_OPEN,CB_RESETCONTENT,0,0);
	for(i=0;i<max_open_with;i++){
		_snprintf(tmp,sizeof(tmp),"%i",i+1);
		SendDlgItemMessage(hwnd,IDC_SELECT_OPEN,CB_ADDSTRING,0,tmp);
	}
	i=0;
	get_ini_value("OPTIONS","select_open_index",&i);
	SendDlgItemMessage(hwnd,IDC_SELECT_OPEN,CB_SETCURSEL,i,0);
	populate_caption_open(hwnd);

	SendDlgItemMessage(hwnd,IDC_COMBO_FONT,CB_RESETCONTENT,0,0);
	add_fonts(hwnd,IDC_COMBO_FONT);
	get_current_font(hwnd,IDC_COMBO_FONT);

	SendDlgItemMessage(hwnd,IDC_LISTBOX_FONT,CB_RESETCONTENT,0,0);
	add_fonts(hwnd,IDC_LISTBOX_FONT);
	get_current_font(hwnd,IDC_LISTBOX_FONT);

	i=20;
	get_ini_value("OPTIONS","match_prefix_len",&i);
	SetDlgItemInt(hwnd,IDC_MATCH_PREFIX_LEN,i,FALSE);
	i=FALSE;
	get_ini_value("OPTIONS","show_column",&i);
	if(i!=0)
		i=BST_CHECKED;
	else
		i=BST_UNCHECKED;
	CheckDlgButton(hwnd,IDC_SHOW_COLUMN,i);
	return TRUE;
}
int save_select_open(HWND hwnd)
{
	char str[10];
	int val;
	str[0]=0;
	GetDlgItemText(hwnd,IDC_SELECT_OPEN,str,sizeof(str));
	val=atoi(str)-1;
	if(val>=0){
		write_ini_value("OPTIONS","select_open_index",val);
		return TRUE;
	}
	return FALSE;
}
int save_options(HWND hwnd)
{
	char str[MAX_PATH*2];
	char key[80];
	int index,val;

	index=SendDlgItemMessage(hwnd,IDC_SELECT_OPEN,CB_GETCURSEL,0,0);
	if(index<0 || index>10)
		index=0;
	index++;
	str[0]=0;
	GetDlgItemText(hwnd,IDC_OPEN1,str,sizeof(str));
	quote_to_pipe_char(str);
	_snprintf(key,sizeof(key),"open%i",index);
	write_ini_str("OPTIONS",key,str);
	str[0]=0;
	GetDlgItemText(hwnd,IDC_CAPTION,str,sizeof(str));
	_snprintf(key,sizeof(key),"caption%i",index);
	write_ini_str("OPTIONS",key,str);

	save_select_open(hwnd);

	str[0]=0;
	GetDlgItemText(hwnd,IDC_MATCH_PREFIX_LEN,str,sizeof(str));
	val=atoi(str);
	if(val<0)val=0;
	if(val>512)val=512;
	write_ini_value("OPTIONS","match_prefix_len",val);

	if(IsDlgButtonChecked(hwnd,IDC_SHOW_COLUMN)==BST_CHECKED)
		val=1;
	else
		val=0;
	write_ini_value("OPTIONS","show_column",val);

	key[0]=0;
	get_dropdown_name(IDC_COMBO_FONT,key,sizeof(key));
	str[0]=0;
	GetDlgItemText(hwnd,IDC_COMBO_FONT,str,sizeof(str));
	write_ini_str("OPTIONS",key,str);

	key[0]=0;
	get_dropdown_name(IDC_LISTBOX_FONT,key,sizeof(key));
	str[0]=0;
	GetDlgItemText(hwnd,IDC_LISTBOX_FONT,str,sizeof(str));
	write_ini_str("OPTIONS",key,str);

	return TRUE;
}
int save_combo_edit_ctrl(HWND hwnd)
{
	int ctrls[4]={IDC_COMBO_SEARCH,IDC_COMBO_REPLACE,IDC_COMBO_MASK,IDC_COMBO_PATH};
	int i,index;
	char str[1024];
	for(i=0;i<sizeof(ctrls)/sizeof(int);i++){
		str[0]=0;
		GetDlgItemText(hwnd,ctrls[i],str,sizeof(str));
		if(strlen(str)>0 && strcmp(str,"Binary mode -->>")!=0){
			index=SendDlgItemMessage(hwnd,ctrls[i],CB_FINDSTRINGEXACT,-1,str);
			if(index<0)
				SendDlgItemMessage(hwnd,ctrls[i],CB_INSERTSTRING,0,str);
			else{
				SendDlgItemMessage(hwnd,ctrls[i],CB_DELETESTRING,index,0);
				SendDlgItemMessage(hwnd,ctrls[i],CB_INSERTSTRING,0,str);
			}
			SendDlgItemMessage(hwnd,ctrls[i],CB_DELETESTRING,max_items_combobox,0);
			SetDlgItemText(hwnd,ctrls[i],str);
		}
	}
	return TRUE;
}
int show_options_help(HWND hwnd)
{
	static int help_active=FALSE;
	if(!help_active){
		help_active=TRUE;
		MessageBox(hwnd,
			"variables:\r\n"
			"%PATH%=full filename with path\r\n"
			"%LINE%,%COL%,%OFFSET%\r\n"
			,"HELP",MB_OK);
		help_active=FALSE;
	}
	return help_active;
}
LRESULT CALLBACK options_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg)
	{
	case WM_INITDIALOG:
		grippy=create_grippy(hwnd);
		init_options_win_anchor(hwnd);
		restore_options_rel_pos(hwnd);
		SendDlgItemMessage(hwnd,IDC_MATCH_PREFIX_LEN,EM_LIMITTEXT,4,0);
		populate_options(hwnd);
		return 0;
	case WM_DESTROY:
		save_options_rel_pos(hwnd);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_options(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_DRAWITEM:
		break;
	case WM_HELP:
		show_options_help(hwnd);
		return TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_SELECT_OPEN:
			if(HIWORD(wparam)==CBN_SELENDOK)
				populate_caption_open(hwnd);
			break;
		case IDC_COMBO_FONT:
			if(HIWORD(wparam)==CBN_SELENDOK){
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_COMBO_FONT,str,sizeof(str));
				font=fontname_to_int(str);
				SendDlgItemMessage(GetParent(hwnd),IDC_COMBO_SEARCH,WM_SETFONT,GetStockObject(font),0);
				SendDlgItemMessage(GetParent(hwnd),IDC_COMBO_REPLACE,WM_SETFONT,GetStockObject(font),0);
				SendDlgItemMessage(GetParent(hwnd),IDC_COMBO_PATH,WM_SETFONT,GetStockObject(font),0);
				SendDlgItemMessage(GetParent(hwnd),IDC_COMBO_MASK,WM_SETFONT,GetStockObject(font),0);
			}
			break;
		case IDC_LISTBOX_FONT:
			if(HIWORD(wparam)==CBN_SELENDOK){
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_LISTBOX_FONT,str,sizeof(str));
				font=fontname_to_int(str);
				SendDlgItemMessage(GetParent(hwnd),IDC_LIST1,WM_SETFONT,GetStockObject(font),0);
				reset_line_width();
				InvalidateRect(GetDlgItem(GetParent(hwnd),IDC_LIST1),NULL,TRUE);
			}
			break;
		case IDC_OPEN_INI:
			open_ini(hwnd,GetKeyState(VK_CONTROL)&0x8000);
			break;
		case IDC_APPLY:
			save_options(hwnd);
			break;
		case IDOK:
			save_options(hwnd);
			EndDialog(hwnd,TRUE);
			return 0;
		case IDCANCEL:
			save_select_open(hwnd);
			EndDialog(hwnd,0);
			return 0;
		}
		break;
	}
	return 0;
}
int do_options_dlg(HWND hwnd)
{
	extern HINSTANCE	ghinstance;
	return DialogBox(ghinstance,MAKEINTRESOURCE(IDD_OPTIONS),hwnd,options_proc);
}