#define WINVER 0x500
#define _WIN32_WINNT 0x500
#include <windows.h>
#include "resource.h"

#define ANCHOR_LEFT 1
#define ANCHOR_RIGHT 2
#define ANCHOR_TOP 4
#define ANCHOR_BOTTOM 8
#define ANCHOR_HCENTER 16

struct CONTROL_ANCHOR{
	int ctrl_id;
	int anchor_mask;
	RECT rect_ctrl,rect_parent;
	int initialized;
};

struct WIN_REL_POS{
	RECT rparent,rwin;
	int initialized;
};

int anchor_init(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len)
{
	int i;
	RECT rparent={0};
	GetClientRect(hparent,&rparent);
	for(i=0;i<clist_len;i++){
		HWND hctrl;
		struct CONTROL_ANCHOR *anchor;
		anchor=&clist[i];
		anchor->rect_parent=rparent;
		hctrl=GetDlgItem(hparent,anchor->ctrl_id);
		if(hctrl){
			RECT rctrl={0};
			GetWindowRect(hctrl,&rctrl);
			MapWindowPoints(NULL,hparent,(LPPOINT)&rctrl,2);
			anchor->rect_ctrl=rctrl;
		}
		anchor->initialized=1;
	}
	return 0;
}

int anchor_resize(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len)
{
	int i;
	RECT rparent={0};
	GetClientRect(hparent,&rparent);
	for(i=0;i<clist_len;i++){
		HWND hctrl;
		struct CONTROL_ANCHOR *anchor;
		anchor=&clist[i];
		if(!anchor->initialized)
			continue;
		hctrl=GetDlgItem(hparent,anchor->ctrl_id);
		if(hctrl){
			int x=0,y=0,cx=0,cy=0,delta;
			int flags=0;
			switch(anchor->anchor_mask){
			case ANCHOR_RIGHT|ANCHOR_TOP:
			case ANCHOR_RIGHT:
				{
					delta=anchor->rect_parent.right-anchor->rect_ctrl.left;
					x=rparent.right-delta;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					flags=SWP_NOSIZE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_BOTTOM:
			case ANCHOR_BOTTOM:
				{
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=rparent.bottom-delta;
					flags=SWP_NOSIZE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					delta=anchor->rect_parent.bottom -  anchor->rect_ctrl.bottom;
					cy=(rparent.bottom-rparent.top) - delta - y;
					flags=SWP_NOMOVE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP:
			case ANCHOR_LEFT|ANCHOR_RIGHT:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					//y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					cy=anchor->rect_ctrl.bottom-anchor->rect_ctrl.top;
					flags=SWP_NOMOVE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					y=rparent.bottom-delta;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					cy=anchor->rect_ctrl.bottom-anchor->rect_ctrl.top;
					flags=SWP_SHOWWINDOW;
				}
				break;
			case ANCHOR_RIGHT|ANCHOR_BOTTOM:
				{
					delta=anchor->rect_parent.right-anchor->rect_ctrl.left;
					x=rparent.right-delta;
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					y=rparent.bottom-delta;
					flags=SWP_NOSIZE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_BOTTOM:
			case ANCHOR_TOP|ANCHOR_BOTTOM:
				{
					//x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					cx=anchor->rect_ctrl.right-anchor->rect_ctrl.left;
					delta=anchor->rect_parent.bottom -  anchor->rect_ctrl.bottom;
					cy=(rparent.bottom-rparent.top) - delta - y;
					flags=SWP_NOMOVE;
				}
				break;
			case ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM:
				{
					delta=anchor->rect_parent.right-anchor->rect_ctrl.left;
					x=rparent.right-delta;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					cx=anchor->rect_ctrl.right-anchor->rect_ctrl.left;
					delta=anchor->rect_parent.bottom -  anchor->rect_ctrl.bottom;
					cy=(rparent.bottom-rparent.top) - delta - y;
					flags=SWP_SHOWWINDOW;
				}
				break;
			case ANCHOR_BOTTOM|ANCHOR_HCENTER:
				{
					delta=rparent.right-rparent.left;
					x=delta/2;
					delta=anchor->rect_ctrl.right-anchor->rect_ctrl.left;
					delta/=2;
					x-=delta;
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					y=rparent.bottom-delta;
					flags=SWP_NOSIZE;
				}
				break;
			}
			if(flags){
				flags&=~SWP_SHOWWINDOW;
				flags|=SWP_NOZORDER|SWP_NOREPOSITION;
				SetWindowPos(hctrl,NULL,x,y,cx,cy,flags);
			}
		}
	}
	return 0;
}

int save_win_rel_position(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos)
{
	int result=FALSE;
	WINDOWPLACEMENT wp={0};
	wp.length=sizeof(WINDOWPLACEMENT);
	if(GetWindowPlacement(hparent,&wp)){
		relpos->rparent=wp.rcNormalPosition;
		if(GetWindowPlacement(hwin,&wp)){
			relpos->rwin=wp.rcNormalPosition;
			result=TRUE;
		}
	}
	relpos->initialized=result;
	return 0;
}
int restore_win_rel_position(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos)
{
	//clamp window to nearest monitor
	if(relpos->initialized){
		RECT rparent;
		if(GetWindowRect(hparent,&rparent)){
			HMONITOR hmon;
			RECT rwin;
			int x,y,cx,cy;
			x=relpos->rwin.left-relpos->rparent.left;
			y=relpos->rwin.top-relpos->rparent.top;
			cx=relpos->rwin.right-relpos->rwin.left;
			cy=relpos->rwin.bottom-relpos->rwin.top;
			rwin.left=rparent.left+x;
			rwin.top=rparent.top+y;
			rwin.right=rwin.left+cx;
			rwin.bottom=rwin.top+cy;
			hmon=MonitorFromRect(&rwin,MONITOR_DEFAULTTONEAREST);
			if(hmon){
				MONITORINFO mi;
				mi.cbSize=sizeof(mi);
				if(GetMonitorInfo(hmon,&mi)){
					RECT rmon;
					rmon=mi.rcWork;
					x=rwin.left;
					y=rwin.top;
					if(x<rmon.left)
						x=rmon.left;
					if(y<rmon.top)
						y=rmon.top;
					if(cx>(rmon.right-rmon.left))
						cx=rmon.right-rmon.left;
					if(cy>(rmon.bottom-rmon.top))
						cy=rmon.bottom-rmon.top;
					if((x+cx)>rmon.right)
						x=rmon.right-cx;
					if((y+cy)>rmon.bottom)
						y=rmon.bottom-cy;
					SetWindowPos(hwin,NULL,x,y,cx,cy,
						SWP_SHOWWINDOW|SWP_NOZORDER);
				}
			}
			
		}
	}
	return 0;
}
int save_ini_win_rel_pos(char *section,struct WIN_REL_POS *relpos)
{
	if(relpos->initialized){
		int offsetx,offsety;
		int width,height;
		offsetx=relpos->rparent.left-relpos->rwin.left;
		offsety=relpos->rparent.top-relpos->rwin.top;
		width=relpos->rwin.right-relpos->rwin.left;
		height=relpos->rwin.bottom-relpos->rwin.top;
		write_ini_value(section,"width",width);
		write_ini_value(section,"height",height);
		write_ini_value(section,"offsetx",offsetx);
		write_ini_value(section,"offsety",offsety);
	}
	return 0;
}
int load_ini_win_rel_pos(char *section,struct WIN_REL_POS *relpos)
{
	int offsetx=0,offsety=0;
	int width=0,height=0;
	get_ini_value(section,"width",&width);
	get_ini_value(section,"height",&height);
	get_ini_value(section,"offsetx",&offsetx);
	get_ini_value(section,"offsety",&offsety);
	if(width>=50 && height>=50){
		relpos->initialized=1;
		relpos->rparent.left=offsetx;
		relpos->rparent.top=offsety;
		relpos->rwin.left=0;
		relpos->rwin.top=0;
		relpos->rwin.right=width;
		relpos->rwin.bottom=height;
	}
	return 0;
}
int snap_window(HWND hwnd,RECT *rect)
{
	if(hwnd && rect){
		HMONITOR hmon;
		MONITORINFO mi;
		hmon=MonitorFromRect(rect,MONITOR_DEFAULTTONEAREST);
		mi.cbSize=sizeof(mi);
		if(GetMonitorInfo(hmon,&mi)){
			long d_top,d_bottom,d_left,d_right;
			d_right=mi.rcWork.right-rect->right;
			if(d_right<=8 && d_right>=-4){
				rect->right=mi.rcWork.right;
				rect->left+=d_right;
			}
			d_left=rect->left-mi.rcWork.left;
			if(d_left<=8 && d_left>=-4){
				rect->left=mi.rcWork.left;
				rect->right-=d_left;
			}
			d_top=rect->top-mi.rcWork.top;
			if(d_top<=8 && d_top>=-4){
				rect->top=mi.rcWork.top;
				rect->bottom-=d_top;
			}
			d_bottom=mi.rcWork.bottom-rect->bottom;
			if(d_bottom<=8 && d_bottom>=-4){
				rect->bottom=mi.rcWork.bottom;
				rect->top+=d_bottom;
			}
		}
	}
	return 0;
}

int snap_sizing(HWND hwnd,RECT *rect,int side)
{
	int result=FALSE;
	if(hwnd && rect){
		HMONITOR hmon;
		MONITORINFO mi;
		hmon=MonitorFromRect(rect,MONITOR_DEFAULTTONEAREST);
		mi.cbSize=sizeof(mi);
		if(GetMonitorInfo(hmon,&mi)){
			RECT *rwork=&mi.rcWork;
			const int snap_size=10;
			if(side==WMSZ_TOP || side==WMSZ_TOPLEFT || side==WMSZ_TOPRIGHT){
				if(abs(rect->top - rwork->top)<snap_size){
					rect->top=rwork->top;
					result=TRUE;
				}
			}
			if(side==WMSZ_BOTTOM || side==WMSZ_BOTTOMLEFT || side==WMSZ_BOTTOMRIGHT){
				if(abs(rect->bottom - rwork->bottom)<snap_size){
					rect->bottom=rwork->bottom;
					result=TRUE;
				}
			}
			if(side==WMSZ_LEFT || side==WMSZ_TOPLEFT || side==WMSZ_BOTTOMLEFT){
				if(abs(rect->left - rwork->left)<snap_size){
					rect->left=rwork->left;
					result=TRUE;
				}
			}
			if(side==WMSZ_RIGHT || side==WMSZ_TOPRIGHT || side==WMSZ_BOTTOMRIGHT){
				if(abs(rect->right - rwork->right)<snap_size){
					rect->right=rwork->right;
					result=TRUE;
				}
			}
		}
	}
	return result;
}
#define GRIPPIE_SQUARE_SIZE 15
int create_grippy(HWND hwnd)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	
	return CreateWindow("Scrollbar",NULL,WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP,
		client_rect.right-GRIPPIE_SQUARE_SIZE,
		client_rect.bottom-GRIPPIE_SQUARE_SIZE,
		GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
		hwnd,NULL,NULL,NULL);
}

int grippy_move(HWND hwnd,HWND grippy)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	if(grippy!=0)
	{
		SetWindowPos(grippy,NULL,
			client_rect.right-GRIPPIE_SQUARE_SIZE,
			client_rect.bottom-GRIPPIE_SQUARE_SIZE,
			GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
			SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	return 0;
}
struct CONTROL_ANCHOR ini_win_anchor[]={
	{IDC_TXT_LOCAL,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP,0,0,0},
	{IDC_TXT_APPDATA,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP,0,0,0},
	{IDC_INSTALL_INFO,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP,0,0,0}
};
int init_ini_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,ini_win_anchor,sizeof(ini_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_ini_win(HWND hwnd)
{
	return anchor_resize(hwnd,ini_win_anchor,sizeof(ini_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}

struct CONTROL_ANCHOR main_win_anchor[]={
	{IDC_COMBO_SEARCH,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_COMBO_REPLACE,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_COMBO_MASK,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_COMBO_PATH,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_SEARCH_OPTIONS,ANCHOR_RIGHT,0,0,0},
	{IDC_REPLACE_OPTIONS,ANCHOR_RIGHT,0,0,0},
	{IDC_FILE_OPTIONS,ANCHOR_RIGHT,0,0,0},
	{IDC_PATH_OPTIONS,ANCHOR_RIGHT,0,0,0},
	{IDC_LIST1,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_STATUS,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0}
};

int init_main_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,main_win_anchor,sizeof(main_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_main_win(HWND hwnd)
{
	return anchor_resize(hwnd,main_win_anchor,sizeof(main_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}

struct CONTROL_ANCHOR cust_text_win_anchor[]={
	{IDC_BIG_EDIT,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDOK,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_EXTENDED,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0}
};
struct WIN_REL_POS cust_text_win_relpos={0};

int init_cust_text_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,cust_text_win_anchor,sizeof(cust_text_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_cust_text_win(HWND hwnd)
{
	return anchor_resize(hwnd,cust_text_win_anchor,sizeof(cust_text_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_cust_win_rel_pos(HWND hwnd)
{
	return save_win_rel_position(GetParent(hwnd),hwnd,&cust_text_win_relpos);
}
int restore_cust_win_rel_pos(HWND hwnd)
{
	return restore_win_rel_position(GetParent(hwnd),hwnd,&cust_text_win_relpos);
}

struct CONTROL_ANCHOR favs_win_anchor[]={
	{IDC_LIST1,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_FAV_EDIT,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_ADD,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_DELETE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_BROWSE_DIR,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_SELECT,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0}
};
struct WIN_REL_POS favs_win_relpos={0};

int init_favs_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,favs_win_anchor,sizeof(favs_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_favs_win(HWND hwnd)
{
	return anchor_resize(hwnd,favs_win_anchor,sizeof(favs_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_favs_win_rel_pos(HWND hwnd)
{
	save_win_rel_position(GetParent(hwnd),hwnd,&favs_win_relpos);
	return save_ini_win_rel_pos("FAVS_WINDOW",&favs_win_relpos);
}
int restore_favs_win_rel_pos(HWND hwnd)
{
	load_ini_win_rel_pos("FAVS_WINDOW",&favs_win_relpos);
	return restore_win_rel_position(GetParent(hwnd),hwnd,&favs_win_relpos);
}

struct CONTROL_ANCHOR options_win_anchor[]={
	{IDC_OPEN1,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_COMBO_FONT,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDC_LISTBOX_FONT,ANCHOR_LEFT|ANCHOR_RIGHT,0,0,0},
	{IDOK,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_OPEN_INI,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0}
};
struct WIN_REL_POS options_win_relpos={0};

int init_options_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,options_win_anchor,sizeof(options_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_options(HWND hwnd)
{
	return anchor_resize(hwnd,options_win_anchor,sizeof(options_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_options_rel_pos(HWND hwnd)
{
	save_win_rel_position(GetParent(hwnd),hwnd,&options_win_relpos);
	return save_ini_win_rel_pos("OPTIONS_WINDOW",&options_win_relpos);
}
int restore_options_rel_pos(HWND hwnd)
{
	load_ini_win_rel_pos("OPTIONS_WINDOW",&options_win_relpos);
	return restore_win_rel_position(GetParent(hwnd),hwnd,&options_win_relpos);
}

struct CONTROL_ANCHOR replace_win_anchor[]={
	{IDC_LIST1,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0},
	{IDC_REPLACE_THIS,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_REPLACE_REST_FILE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_REPLACE_ALL,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_SKIPTHIS,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_SKIP_REST_FILE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_CANCEL_REMAINING,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0}
};
struct WIN_REL_POS replace_win_relpos={0};

int init_replace_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,replace_win_anchor,sizeof(replace_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_replace(HWND hwnd)
{
	return anchor_resize(hwnd,replace_win_anchor,sizeof(replace_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_replace_rel_pos(HWND hwnd)
{
	save_win_rel_position(GetParent(hwnd),hwnd,&replace_win_relpos);
	return save_ini_win_rel_pos("REPLACE_WINDOW",&replace_win_relpos);
}
int restore_replace_rel_pos(HWND hwnd)
{
	load_ini_win_rel_pos("REPLACE_WINDOW",&replace_win_relpos);
	return restore_win_rel_position(GetParent(hwnd),hwnd,&replace_win_relpos);
}

struct CONTROL_ANCHOR search_progress_anchor[]={
	{IDC_SEARCH_STATUS,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT,0,0,0},
	{IDC_PROGRESS1,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_RIGHT,0,0,0},
	{IDCANCEL,ANCHOR_BOTTOM|ANCHOR_HCENTER,0,0,0},
};
struct WIN_REL_POS search_prog_win_relpos={0};

int init_search_prog_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,search_progress_anchor,sizeof(search_progress_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_search_prog(HWND hwnd)
{
	return anchor_resize(hwnd,search_progress_anchor,sizeof(search_progress_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_search_prog_rel_pos(HWND hwnd)
{
	save_win_rel_position(GetParent(hwnd),hwnd,&search_prog_win_relpos);
	return save_ini_win_rel_pos("SEARCH_PROG_WINDOW",&search_prog_win_relpos);
}
int restore_search_prog_rel_pos(HWND hwnd)
{
	load_ini_win_rel_pos("SEARCH_PROG_WINDOW",&search_prog_win_relpos);
	return restore_win_rel_position(GetParent(hwnd),hwnd,&search_prog_win_relpos);
}

struct CONTROL_ANCHOR context_win_anchor[]={
	{IDC_ROWNUMBER,ANCHOR_LEFT|ANCHOR_TOP|ANCHOR_BOTTOM,0,0,0},
	{IDC_CONTEXT,ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM,0,0,0},
	{IDC_CONTEXT_SCROLLBAR,ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM,0,0,0}
};
struct WIN_REL_POS context_win_relpos={0};

int init_context_win_anchor(HWND hwnd)
{
	return anchor_init(hwnd,context_win_anchor,sizeof(context_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int resize_context(HWND hwnd)
{
	return anchor_resize(hwnd,context_win_anchor,sizeof(context_win_anchor)/sizeof(struct CONTROL_ANCHOR));
}
int save_context_rel_pos(HWND hwnd)
{
	save_win_rel_position(GetParent(hwnd),hwnd,&context_win_relpos);
	return save_ini_win_rel_pos("CONTEXT_WINDOW",&context_win_relpos);
}
int restore_context_rel_pos(HWND hwnd)
{
	load_ini_win_rel_pos("CONTEXT_WINDOW",&context_win_relpos);
	return restore_win_rel_position(GetParent(hwnd),hwnd,&context_win_relpos);
}
int set_context_divider(HWND hwnd,int xpos)
{
	int i;
	if(xpos<0)
		return 0;
	for(i=0;i<sizeof(context_win_anchor)/sizeof(struct CONTROL_ANCHOR);i++){
		int id=context_win_anchor[i].ctrl_id;
		struct CONTROL_ANCHOR *ca=&context_win_anchor[i];
		if(IDC_ROWNUMBER==id){
			ca->rect_ctrl.right=xpos;
		}
		else if(IDC_CONTEXT==id){
			RECT rect={0};
			int w,delta;
			GetClientRect(hwnd,&rect);
			w=rect.right-rect.left;
			delta=w-(ca->rect_parent.right-ca->rect_parent.left);
			ca->rect_ctrl.left=xpos-delta+8;
		}
	}
	resize_context(hwnd);
	return 0;
}