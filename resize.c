#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "resource.h"


enum{
	CONTROL_ID,
	XPOS,YPOS,
	WIDTH,HEIGHT,
	HUG_L,
	HUG_R,
	HUG_T,
	HUG_B,
	HUG_CTRL_X,
	HUG_CTRL_Y,
	HUG_HEIGHT,
	HUG_WIDTH,
	HUG_CTRL_TXT_X,
	HUG_CTRL_TXT_Y,
	HUG_CTRL_TXT_X_,
	HUG_CTRL_TXT_Y_,
	SIZE_HEIGHT_OFF,
	SIZE_WIDTH_OFF,
	SIZE_HEIGHT_PER,
	SIZE_WIDTH_PER,
	SIZE_TEXT_CTRL,
	CONTROL_FINISH,
	RESIZE_FINISH
};

int process_anchor_list(HWND hwnd,short *list)
{
	int limit=9999;
	int i=0,j,x,y,width,height;
	HWND dlg_item;
	HDC	 hdc;
	RECT crect;
	SIZE text_size;
	char str[255];
	double f;
	int done=FALSE;
	int last_text=0;

	memset(&crect,0,sizeof(crect));
	hdc=GetDC(hwnd);
	GetClientRect(hwnd, &crect);
	do{
		switch(list[i]){
		case CONTROL_ID:
			x=y=width=height=0;
			dlg_item=GetDlgItem(hwnd,list[i+1]);
			if(dlg_item==NULL)
				done=TRUE;
			break;
		case XPOS:
			x+=list[i+1];
			break;
		case YPOS:
			y+=list[i+1];
			break;
		case WIDTH:
			width+=list[i+1];
			break;
		case HEIGHT:
			height+=list[i+1];
			break;
		case HUG_L:
			x+=crect.left+list[i+1];
			break;
		case HUG_R:
			x+=crect.right+list[i+1];
			break;
		case HUG_T:
			y+=crect.top+list[i+1];
			break;
		case HUG_B:
			y+=crect.bottom+list[i+1];
			break;
		case HUG_CTRL_X:
			break;
		case HUG_CTRL_Y:
			break;
		case HUG_HEIGHT:
			j=crect.bottom-crect.top;
			f=(double)list[i+1]/1000.0;
			y+=j*f;
			break;
		case HUG_WIDTH:
			j=crect.right-crect.left;
			f=(double)list[i+1]/1000.0;
			x+=j*f;
			break;
		case HUG_CTRL_TXT_X:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			x+=text_size.cx;
			break;
		case HUG_CTRL_TXT_X_:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			x-=text_size.cx;
			break;
		case HUG_CTRL_TXT_Y:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			y+=text_size.cy;
			break;
		case HUG_CTRL_TXT_Y_:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			y-=text_size.cy;
			break;
		case SIZE_HEIGHT_OFF:
			height+=crect.bottom-crect.top+list[i+1];
			break;
		case SIZE_WIDTH_OFF:
			width+=crect.right-crect.left+list[i+1];
			break;
		case SIZE_HEIGHT_PER:
			j=crect.bottom-crect.top;
			f=(double)list[i+1]/1000.0;
			height+=f*j;
			break;
		case SIZE_WIDTH_PER:
			j=crect.right-crect.left;
			f=(double)list[i+1]/1000.0;
			width+=f*j;
			break;
		case SIZE_TEXT_CTRL:
			if(last_text!=list[i+1]){
				GetDlgItemText(hwnd,list[i+1],str,sizeof(str)-1);
				GetTextExtentPoint32(hdc,str,strlen(str),&text_size);
				last_text=list[i+1];
			}
			width+=text_size.cx;
			height+=text_size.cy;
			break;
		case CONTROL_FINISH:
			SetWindowPos(dlg_item,NULL,x,y,width,height,SWP_NOZORDER);
			break;
		case RESIZE_FINISH:
			done=TRUE;
			break;
		default:
			printf("bad command %i\n",list[i]);
			break;
		}
		i+=2;
		if(i>limit)
			done=TRUE;
	}while(!done);
	ReleaseDC(hwnd,hdc);
	return TRUE;
}
short win_main_list[]={

	CONTROL_ID,IDC_COMBO_SEARCH,
			XPOS,70,YPOS,47, /*x.off=-542 y.off=-350*/
			SIZE_WIDTH_OFF,-95,HEIGHT,21, /*w.off=-155 h.off=-376*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_COMBO_REPLACE,
			XPOS,70,YPOS,73, /*x.off=-542 y.off=-324*/
			SIZE_WIDTH_OFF,-95,HEIGHT,21, /*w.off=-155 h.off=-376*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_COMBO_MASK,
			XPOS,70,YPOS,98, /*x.off=-542 y.off=-299*/
			SIZE_WIDTH_OFF,-95,HEIGHT,21, /*w.off=-155 h.off=-376*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_COMBO_PATH,
			XPOS,70,YPOS,124, /*x.off=-542 y.off=-273*/
			SIZE_WIDTH_OFF,-95,HEIGHT,21, /*w.off=-155 h.off=-376*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_SEARCH_OPTIONS,
			HUG_R,-21,YPOS,46, /*x.off=-83 y.off=-351*/
			WIDTH,21,HEIGHT,21, /*w.off=-581 h.off=-374*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_REPLACE_OPTIONS,
			HUG_R,-21,YPOS,72, /*x.off=-83 y.off=-325*/
			WIDTH,21,HEIGHT,21, /*w.off=-581 h.off=-374*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_FILE_OPTIONS,
			HUG_R,-21,YPOS,96, /*x.off=-83 y.off=-301*/
			WIDTH,21,HEIGHT,21, /*w.off=-579 h.off=-374*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_PATH_OPTIONS,
			HUG_R,-21,YPOS,122, /*x.off=-83 y.off=-275*/
			WIDTH,21,HEIGHT,21, /*w.off=-578 h.off=-374*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_CHECK_DEPTH,
			WIDTH,80,HEIGHT,21,
			XPOS,150,
			YPOS,148,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_DEPTH_LEVEL,
			WIDTH,40,HEIGHT,18,
			XPOS,250,
			YPOS,148,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_LIST1,
			XPOS,0,YPOS,170, /*x.off=-614 y.off=-225*/
			//WIDTH,605,HEIGHT,192, /*w.off=-9 h.off=-205*/
			SIZE_WIDTH_OFF,0,SIZE_HEIGHT_OFF,-192,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_STATUS,
			XPOS,0,HUG_B,-21, /*x.off=-605 y.off=-23*/
			SIZE_WIDTH_OFF,-20,HEIGHT,21,
			/*w.off=-74 h.off=-368*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
short win_search_list[]={
	CONTROL_ID,IDC_SEARCH_STATUS,
			XPOS,0,YPOS,8, /*x.off=-279 y.off=-145*/
			SIZE_WIDTH_OFF,0,SIZE_HEIGHT_OFF,-90, /*w.off=0 h.off=-109*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_PROGRESS1,
			XPOS,0,HUG_B,-78, /*x.off=-279 y.off=-78*/
			SIZE_WIDTH_OFF,0,HEIGHT,23, /*w.off=0 h.off=-130*/
			CONTROL_FINISH,-1,
	CONTROL_ID,2,
			HUG_WIDTH,400,HUG_B,-33, /*x.off=-177 y.off=-33*/
			WIDTH,75,HEIGHT,23, /*w.off=-204 h.off=-130*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
short win_context_list[]={
	CONTROL_ID,IDC_ROWNUMBER,
			XPOS,0,YPOS,0, /*x.off=-606 y.off=-504*/
			WIDTH,90,SIZE_HEIGHT_OFF,0, /*w.off=-516 h.off=0*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_CONTEXT,
			XPOS,95,YPOS,0, /*x.off=-514 y.off=-504*/
			SIZE_WIDTH_OFF,-95,SIZE_HEIGHT_OFF,0, /*w.off=-109 h.off=0*/
			WIDTH,-15,
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_CONTEXT_SCROLLBAR,
			HUG_R,-15,YPOS,0, /*x.off=-15 y.off=-504*/
			WIDTH,15,SIZE_HEIGHT_OFF,-18, /*w.off=-591 h.off=-18*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
short win_replace_list[]={
	CONTROL_ID,IDC_REPLACE_THIS,
			XPOS,57,HUG_B,-77, /*x.off=-465 y.off=-77*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_REPLACE_REST_FILE,
			XPOS,195,HUG_B,-77, /*x.off=-327 y.off=-77*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_REPLACE_ALL,
			XPOS,333,HUG_B,-77, /*x.off=-189 y.off=-77*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_SKIPTHIS,
			XPOS,57,HUG_B,-34, /*x.off=-465 y.off=-34*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_SKIP_REST_FILE,
			XPOS,195,HUG_B,-34, /*x.off=-327 y.off=-34*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_CANCEL_REMAINING,
			XPOS,333,HUG_B,-34, /*x.off=-189 y.off=-34*/
			WIDTH,114,HEIGHT,23, /*w.off=-408 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_LIST1,
			XPOS,0,YPOS,3, /*x.off=-522 y.off=-267*/
			SIZE_WIDTH_OFF,0,SIZE_HEIGHT_OFF,-90, /*w.off=0 h.off=-90*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
short win_options_list[]={
	CONTROL_ID,IDC_SELECT_OPEN,
			XPOS,143,YPOS,8, /*x.off=-244 y.off=-262*/
			WIDTH,89,HEIGHT,21, /*w.off=-298 h.off=-249*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_CAPTION,
			XPOS,9,YPOS,55, /*x.off=-378 y.off=-215*/
			WIDTH,125,HEIGHT,29, /*w.off=-262 h.off=-241*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_OPEN1,
			XPOS,143,YPOS,55, /*x.off=-244 y.off=-215*/
			SIZE_WIDTH_OFF,-144,HEIGHT,29, /*w.off=-144 h.off=-241*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_APPLY,
			XPOS,11,YPOS,94, /*x.off=-376 y.off=-176*/
			WIDTH,75,HEIGHT,23, /*w.off=-312 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_FNAME_COLOR,
			XPOS,95,YPOS,115, /*x.off=-292 y.off=-155*/
			WIDTH,87,HEIGHT,23, /*w.off=-300 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_MATCH_COLOR,
			XPOS,218,YPOS,114, /*x.off=-169 y.off=-156*/
			WIDTH,87,HEIGHT,23, /*w.off=-300 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_COMBO_FONT,
			XPOS,95,YPOS,145, /*x.off=-292 y.off=-125*/
			SIZE_WIDTH_OFF,-96,HEIGHT,21, /*w.off=-96 h.off=-249*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_LISTBOX_FONT,
			XPOS,95,YPOS,179, /*x.off=-292 y.off=-91*/
			SIZE_WIDTH_OFF,-96,HEIGHT,21, /*w.off=-96 h.off=-249*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH,
	CONTROL_ID,IDC_HIT_WIDTH,
			XPOS,104,YPOS,210, /*x.off=-283 y.off=-60*/
			WIDTH,60,HEIGHT,23, /*w.off=-327 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_SHOW_COLUMN,
			XPOS,185,YPOS,211, /*x.off=-202 y.off=-59*/
			WIDTH,90,HEIGHT,16, /*w.off=-297 h.off=-254*/
			CONTROL_FINISH,-1,
	CONTROL_ID,1,
			XPOS,0,YPOS,247, /*x.off=-387 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-312 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_OPEN_INI,
			XPOS,104,YPOS,245, /*x.off=-283 y.off=-25*/
			WIDTH,75,HEIGHT,23, /*w.off=-312 h.off=-247*/
			CONTROL_FINISH,-1,
	CONTROL_ID,2,
			XPOS,278,YPOS,247, /*x.off=-109 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-312 h.off=-247*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
short win_favs_list[]={
	CONTROL_ID,IDC_LIST1,
			XPOS,0,YPOS,0, /*x.off=-452 y.off=-171*/
			SIZE_WIDTH_OFF,0,SIZE_HEIGHT_OFF,-49, /*w.off=0 h.off=-41*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_FAV_EDIT,
			XPOS,0,HUG_B,-47, /*x.off=-452 y.off=-43*/
			SIZE_WIDTH_OFF,0,HEIGHT,23, /*w.off=-2 h.off=-148*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_ADD,
			XPOS,0,HUG_B,-23, /*x.off=-452 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-377 h.off=-148*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_DELETE,
			XPOS,107,HUG_B,-23, /*x.off=-345 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-377 h.off=-148*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_BROWSE_DIR,
			XPOS,209,HUG_B,-23, /*x.off=-243 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-377 h.off=-148*/
			CONTROL_FINISH,-1,
	CONTROL_ID,IDC_SELECT,
			XPOS,284,HUG_B,-23, /*x.off=-168 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-377 h.off=-148*/
			CONTROL_FINISH,-1,
	CONTROL_ID,2,
			XPOS,359,HUG_B,-23, /*x.off=-93 y.off=-23*/
			WIDTH,75,HEIGHT,23, /*w.off=-377 h.off=-148*/
			CONTROL_FINISH,-1,
	RESIZE_FINISH
};
int get_idc_name(int idc,char *name,int len)
{
	int found=FALSE;
	FILE *f;
	char str[1024];
	f=fopen("resource.h","rt");
	if(f!=0){
		memset(str,0,sizeof(str));
		while(fgets(str,sizeof(str)-1,f)){
			int id=0;
			char t[256]={0};
			sscanf(str,"#define %s %i",t,&id);
			if(idc==id){
				strncpy(name,t,len);
				found=TRUE;
				break;
			}
		}
		fclose(f);
	}
	return found;
}
int dump_sizes(HWND hwnd,short *IDC)
{
	int i;
	RECT client,win;
	GetClientRect(hwnd,&client);
	for(i=0;i<1000;i++){
		POINT p={0};
		int width,height;
		int cw,ch;
		char name[256];
		if(IDC[i]==RESIZE_FINISH)
			break;
		if(IDC[i]==CONTROL_ID)
			i++;
		else{
			i++;
			continue;
		}
		sprintf(name,"%i",IDC[i]);
		get_idc_name(IDC[i],name,sizeof(name));
		printf("CONTROL_ID,%s,\n",name);
		GetWindowRect(GetDlgItem(hwnd,IDC[i]),&win);
		p.x=win.left;
		p.y=win.top;
		ScreenToClient(hwnd,&p);
		width=win.right-win.left;
		height=win.bottom-win.top;
		cw=client.right-client.left;
		ch=client.bottom-client.top;
		printf("\tXPOS,%i,YPOS,%i, /*x.off=%i y.off=%i*/\n",
			p.x,p.y,
			-client.right+p.x,-client.bottom+p.y);
		printf("\tWIDTH,%i,HEIGHT,%i, /*w.off=%i h.off=%i*/\n\tCONTROL_FINISH,-1,\n",
			width,height,
			-cw+width,-ch+height);

	}
	printf("\n");return 0;
}


int reposition_controls(HWND hwnd, int *list)
{
	process_anchor_list(hwnd,list);
	return TRUE;
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
int get_word(char *str,int num,char *out,int olen)
{
	int i,len,count=0,index=0;
	int start_found=FALSE;
	len=strlen(str);
	for(i=0;i<len;i++){
		if(str[i]==','){
			count++;
			if(count>num){
				out[index++]=0;
				break;
			}
		}
		if(!start_found){
			if(count==num && str[i]>' ' && str[i]!=',')
				start_found=TRUE;
		}
		if(start_found){
			if(str[i]<' ' || str[i]=='/'){
				out[index++]=0;
				break;
			}
			if(str[i]>' ')
				out[index++]=str[i];
			if(i==len-1){
				out[index++]=0;
				break;
			}
		}
		if(index>=olen-2){
			out[index++]=0;
			break;
		}
	}
	return TRUE;
}
int modify_list(short *list)
{
	FILE *f;
	char str[1024];
	char *tags[]={
	"CONTROL_ID",
	"XPOS","YPOS",
	"WIDTH","HEIGHT",
	"HUG_L",
	"HUG_R",
	"HUG_T",
	"HUG_B",
	"HUG_CTRL_X",
	"HUG_CTRL_Y",
	"HUG_HEIGHT",
	"HUG_WIDTH",
	"HUG_CTRL_TXT_X",
	"HUG_CTRL_TXT_Y",
	"HUG_CTRL_TXT_X_",
	"HUG_CTRL_TXT_Y_",
	"SIZE_HEIGHT_OFF",
	"SIZE_WIDTH_OFF",
	"SIZE_HEIGHT_PER",
	"SIZE_WIDTH_PER",
	"SIZE_TEXT_CTRL",
	"CONTROL_FINISH",
	"RESIZE_FINISH",
	0
	};
	if(list==0)
		return;
	f=fopen("rc.txt","rb");
	if(f!=0){
		int result=FALSE;
		int index=0;
		int done=FALSE;
		printf("-------start---------\n");
		do{
			char *s;
			int i,j;
			str[0]=0;
			result=fgets(str,sizeof(str),f);
			s=strstr(str,"/");
			if(s!=0)
				s[0]=0;
			for(i=0;i<10;i+=2){
				char word[40]={0};
				if(done)
					break;
				get_word(str,i,word,sizeof(word));
				if(strlen(word)==0)
					break;
				if(strlen(word)>0){
					for(j=0;j<100;j++){
						if(tags[j]==0)
							break;
						if(strcmp(word,tags[j])==0){
							int val;
							if(strcmp(word,"CONTROL_ID")==0){
								char w2[40]={0};
								get_word(str,i+1,w2,sizeof(w2));
								printf("%s,%s\n",word,w2);
								index+=2;
								break;
							}
							if(strcmp(word,"CONTROL_FINISH")==0){
								index+=2;
								break;
							}
							if(strcmp(word,"RESIZE_FINISH")==0){
								done=TRUE;
								break;
							}
							if(list[index]==RESIZE_FINISH){
								done=TRUE;
								break;
							}
							list[index++]=j;
							printf("%s,",word);
							word[0]=0;
							get_word(str,i+1,word,sizeof(word));
							printf("%s\n",word);
							if(strlen(word)>0){
								val=atoi(word);
								list[index++]=val;
							}
							else
								index=index;
						}
					}
				}
			}
			if(done)
				break;
		}while(result);
		fclose(f);
	}
	return TRUE;
}
int set_context_divider(int width)
{
	win_context_list[7]=width;
	win_context_list[15]=width+5;
	win_context_list[19]=-(width+5);
	return TRUE;
}
int resize_main(HWND hwnd)
{
	return reposition_controls(hwnd,win_main_list);
}
int resize_search(HWND hwnd)
{
	return reposition_controls(hwnd,win_search_list);
}
int resize_context(HWND hwnd)
{
	return reposition_controls(hwnd,win_context_list);
}
int resize_options(HWND hwnd)
{
	return reposition_controls(hwnd,win_options_list);
}
int resize_replace(HWND hwnd)
{
	return reposition_controls(hwnd,win_replace_list);;
}
int resize_favs(HWND hwnd)
{
//	modify_list(win_favs_list);
	return reposition_controls(hwnd,win_favs_list);
}
int dump_replace(HWND hwnd)
{
	return dump_sizes(hwnd,win_replace_list);
}
int dump_context(HWND hwnd)
{
	return dump_sizes(hwnd,win_context_list);
}
int dump_search(HWND hwnd)
{
	return dump_sizes(hwnd,win_search_list);
}
int dump_main(HWND hwnd)
{
	return dump_sizes(hwnd,win_main_list);
}
int dump_options(HWND hwnd)
{
	return dump_sizes(hwnd,win_options_list);
}
int dump_favs(HWND hwnd)
{
	return dump_sizes(hwnd,win_favs_list);
}