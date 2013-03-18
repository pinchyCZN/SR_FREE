#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include "resource.h"

HBITMAP hcollapse,hexpanded,hline,hfound,hempty,hcontinue;
_CRTIMP char *  __cdecl strstri(const char *, const char *);

COLORREF file_text_color=RGB(255,0,0);
COLORREF match_text_color=RGB(255,0,0);
int line_width=0;
int show_column=TRUE;


int set_scroll_width(HWND hwnd,int control)
{
	SendDlgItemMessage(hwnd,control,LB_SETHORIZONTALEXTENT,line_width,0);
	return TRUE;
}
int set_list_width(int x)
{
	if(x>line_width)
		line_width=x;
	return TRUE;
}
int reset_line_width()
{
	line_width=0;
	return TRUE;
}
int set_show_column(int show)
{
	return show_column=show;
}

int load_bitmaps(HINSTANCE ghinstance)
{
	hcollapse=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_COLLAPSED));
	hexpanded=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_EXPANDED));
	hline=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_LINE));
	hfound=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_FOUND));
	hempty=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_EMPTY));
	hcontinue=LoadBitmap(ghinstance,MAKEINTRESOURCE(IDB_CONTINUE));
	return TRUE;
}
int draw_item(DRAWITEMSTRUCT *di,char *list_string)
{
	int style;
	RECT rect;
	SIZE size;
    HDC hdcMem;
    HBITMAP hbmpOld;
	int bgcolor=COLOR_BACKGROUND;
	int is_file=FALSE,is_line=FALSE,is_offset=FALSE;


	hdcMem=CreateCompatibleDC(di->hDC);
	if(strnicmp(list_string,"file",sizeof("file")-1)==0)
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hexpanded);
	else if(strnicmp(list_string,"line",sizeof("line")-1)==0)
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hline);
	else if(strnicmp(list_string,"offset",sizeof("offset")-1)==0)
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hline);
	else if(strnicmp(list_string,"found",sizeof("found")-1)==0)
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hfound);
	else if(strnicmp(list_string,"replace",sizeof("replace")-1)==0)
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hline);
	else if(list_string[0]==' ')
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hcontinue);
	else
		hbmpOld=(HBITMAP)SelectObject(hdcMem,hempty);
	if(di->itemState&ODS_SELECTED)
		bgcolor=COLOR_HIGHLIGHT;
	else
		bgcolor=COLOR_WINDOW;
	FillRect(di->hDC,&di->rcItem,bgcolor+1);
	BitBlt(di->hDC,di->rcItem.left,di->rcItem.top,di->rcItem.left+16,di->rcItem.top+16,hdcMem,0,0,SRCINVERT);
	rect=di->rcItem;
	rect.left+=16;
//		DrawText(di->hDC,text,-1,&rect,style);
//	SetTextColor(di->hDC,(0xFFFFFF^GetSysColor(COLOR_BTNTEXT)));
	SetTextColor(di->hDC,GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT));
	SetBkColor(di->hDC,GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW));
	style=DT_LEFT|DT_NOPREFIX;
	if(strnicmp(list_string,"file ",sizeof("file ")-1)==0)
		is_file=TRUE;
	else if(strnicmp(list_string,"line ",sizeof("line ")-1)==0)
		is_line=TRUE;
	else if(strnicmp(list_string,"offset ",sizeof("offset ")-1)==0)
		is_offset=TRUE;

	if(is_file){
		char *start;
		GetTextExtentPoint32(di->hDC,list_string,sizeof("file ")-1,&size);
		rect.right=rect.left+size.cx;
		DrawText(di->hDC,list_string,sizeof("file ")-1,&rect,style);

		start=list_string+sizeof("file ")-1;
		GetTextExtentPoint32(di->hDC,start,strlen(start),&size);
		rect.left=rect.right;
		rect.right=rect.left+size.cx;
		SetTextColor(di->hDC,file_text_color);
		DrawText(di->hDC,start,-1,&rect,style);
	}
	else if(is_line || is_offset){
		char *start;
		int col,mlen;
		start=list_string;
		if(is_offset || show_column)
			mlen=strstr(start,"=")-start; //- to view whole string
		else
			mlen=strstri(start,"col")-start;

		if(mlen<5 || mlen>100)
			mlen=5;
		GetTextExtentPoint32(di->hDC,list_string,mlen,&size);
		rect.right=rect.left+size.cx;
		DrawText(di->hDC,start,mlen,&rect,style);

		//draw dash or first char after line info
		GetTextExtentPoint32(di->hDC,"-",1,&size);
		rect.left=rect.right;
		rect.right=rect.left+size.cx;
		DrawText(di->hDC,"-",1,&rect,style);

		col=0,mlen=0;
		if(is_line){
							//"Line %I64i col %I64i = %i %i %I64X -%s"
			sscanf(list_string,"%*s %*s %*s %*s %*s %i %i",&col,&mlen);
		}
		else{
							//"Offset 0x%I64X = %I64i %i %i -%s"
			sscanf(list_string,"%*s %*s %*s %*s %i %i",&col,&mlen);
		}
		start=strstr(list_string,"-");
		if(start!=0){
			int len;
			start++;
			len=strlen(start);
			if(col<0)
				col=0;
			if(col+mlen>len){
				mlen=len-col;
				if(mlen<0 || mlen>len)
					mlen=len;
			}
			if(col<=0){
				//draw match
				GetTextExtentPoint32(di->hDC,start,mlen,&size);
				rect.left=rect.right;
				rect.right=rect.left+size.cx;
				SetTextColor(di->hDC,match_text_color);
				DrawText(di->hDC,start,-1,&rect,style);
				//draw rest
				start+=mlen;
				GetTextExtentPoint32(di->hDC,start,strlen(start),&size);
				rect.left=rect.right;
				rect.right=rect.left+size.cx;
				SetTextColor(di->hDC,GetSysColor(COLOR_WINDOWTEXT));
				DrawText(di->hDC,start,-1,&rect,style);
			}
			else{
				//col-=1; //cols=1-n array=0-n
				//draw text
				GetTextExtentPoint32(di->hDC,start,col,&size);
				rect.left=rect.right;
				rect.right=rect.left+size.cx;
				DrawText(di->hDC,start,col,&rect,style);

				//draw match
				start+=col;
				GetTextExtentPoint32(di->hDC,start,mlen,&size);
				rect.left=rect.right;
				rect.right=rect.left+size.cx;
				SetTextColor(di->hDC,match_text_color);
				DrawText(di->hDC,start,mlen,&rect,style);

				//draw text
				start+=mlen;
				GetTextExtentPoint32(di->hDC,start,strlen(start),&size);
				rect.left=rect.right;
				rect.right=rect.left+size.cx;
				SetTextColor(di->hDC,GetSysColor(COLOR_WINDOWTEXT));
				DrawText(di->hDC,start,strlen(start),&rect,style);
			}
		}

	}
	else{
		GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
		rect.right=rect.left+size.cx;
		DrawText(di->hDC,list_string,-1,&rect,style);
	}
	if(di->itemState&ODS_FOCUS)
		DrawFocusRect(di->hDC,&di->rcItem); 

	/*
	switch(di->itemAction){
	case ODA_DRAWENTIRE:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_FOCUS:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_SELECT:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	}
	*/
	GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	set_list_width(size.cx+16);
	SelectObject(hdcMem, hbmpOld); 
    DeleteDC(hdcMem);
	return TRUE;
}
int list_drawitem(HWND hwnd,int id,DRAWITEMSTRUCT *di)
{
	char text[1024];
	if(FALSE)
	{
		{
			static DWORD tick=0;
			if((GetTickCount()-tick)>500)
				printf("--\n");
			tick=GetTickCount();
		}
		{
			char s1[100]={0},s2[100]={0};
			int c=0;
			if(di->itemAction&ODA_DRAWENTIRE)
				c++;
			if(di->itemAction&ODA_SELECT)
				c++;
			if(di->itemAction&ODS_FOCUS)
				c++;
			if(c>1)
				c=c;
			if(di->itemAction&ODA_DRAWENTIRE)
				strcat(s1,"ODA_DRAWENTIRE|");
			if(di->itemAction&ODA_FOCUS)
				strcat(s1,"ODA_FOCUS|");
			if(di->itemAction&ODA_SELECT)
				strcat(s1,"ODA_SELECT|");
			if(di->itemState&ODS_DEFAULT)
				strcat(s2,"ODS_DEFAULT|");
			if(di->itemState&ODS_DISABLED)
				strcat(s2,"ODS_DISABLED|");
			if(di->itemState&ODS_FOCUS)
				strcat(s2,"ODS_FOCUS|");
			if(di->itemState&ODS_GRAYED)
				strcat(s2,"ODS_GRAYED|");
			if(di->itemState&ODS_SELECTED)
				strcat(s2,"ODS_SELECTED|");
			printf("%2i %s\t%s\n",di->itemID,s1,s2);
		}
	}
	switch(di->itemAction){
	case ODA_DRAWENTIRE:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
	case ODA_FOCUS:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
		break;
	case ODA_SELECT:
		text[0]=0;
		SendDlgItemMessage(hwnd,id,LB_GETTEXT,di->itemID,text);
		draw_item(di,text);
		break;

	}
	return TRUE;
}