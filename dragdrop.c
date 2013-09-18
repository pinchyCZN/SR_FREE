#include <windows.h>
#include <commctrl.h>
#include <oleidl.h>

static 	IDropTarget droptarget;
static HWND ghwnd=0;
static HWND hwndTT=0;

int create_tooltip(HWND hwnd,char *tt_text,int x, int y)
{
	if((hwndTT==0) && (tt_text[0]!=0)){
		hwndTT=CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS,NULL,
			WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,        
			CW_USEDEFAULT,CW_USEDEFAULT,
			CW_USEDEFAULT,CW_USEDEFAULT,
			hwnd,NULL,NULL,NULL);
		if(hwndTT!=0){
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_IDISHWND|TTF_TRACK|TTF_ABSOLUTE;
			ti.hwnd = hwndTT;
			ti.uId = hwndTT;
			ti.lpszText = tt_text;
			SendMessage(hwndTT,TTM_ADDTOOL,0,&ti);
			SendMessage(hwndTT,TTM_UPDATETIPTEXTA,0,&ti);
			SendMessage(hwndTT,TTM_SETMAXTIPWIDTH,0,640); //makes multiline tooltips
			SendMessage(hwndTT,TTM_TRACKPOSITION,0,MAKELONG(x,y)); 
			SendMessage(hwndTT,TTM_TRACKACTIVATE,TRUE,&ti);
		}
	}
	return hwndTT;
}
int update_tooltip_text(char *tt_text)
{
	int result=0;
	if(hwndTT!=0){
		TOOLINFO ti;
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_IDISHWND|TTF_TRACK|TTF_ABSOLUTE;
		ti.hwnd = hwndTT;
		ti.uId = hwndTT;
		ti.lpszText = tt_text;
		result=SendMessage(hwndTT,TTM_UPDATETIPTEXTA,0,&ti);
	}
	return result;
}
int destroy_tooltip()
{
	if(hwndTT!=0){
		DestroyWindow(hwndTT);
		hwndTT=0;
	}
	return hwndTT;
}
int get_correct_msg(char *msg,int len)
{
	int shift=FALSE,ctrl=FALSE;
	static char *cmsg="CTRL=mask exact filename";
	static char *smsg="SHIFT=mask *.*";
	static char *csmsg="CTRL+SHIFT mask by *.ext";

	if(msg==0 || len<=0)
		return FALSE;
	shift=GetKeyState(VK_SHIFT)&0x8000;
	ctrl=GetKeyState(VK_CONTROL)&0x8000;
	if(ctrl==FALSE && shift==FALSE)
		_snprintf(msg,len,"%s\r\n%s\r\n%s",cmsg,smsg,csmsg);
	else if(ctrl && shift==FALSE)
		_snprintf(msg,len,"%s",cmsg);
	else if(shift && ctrl==FALSE)
		_snprintf(msg,len,"%s",smsg);
	else
		_snprintf(msg,len,"%s",csmsg);
	msg[len-1]=0;
	return TRUE;
}

//	IUnknown::AddRef
static ULONG STDMETHODCALLTYPE idroptarget_addref (IDropTarget* This)
{
  return S_OK;
}

//	IUnknown::QueryInterface
static HRESULT STDMETHODCALLTYPE
idroptarget_queryinterface (IDropTarget *This,
			       REFIID          riid,
			       LPVOID         *ppvObject)
{
  return S_OK;
}


//	IUnknown::Release
static ULONG STDMETHODCALLTYPE
idroptarget_release (IDropTarget* This)
{
  return S_OK;
}

//	IDropTarget::DragEnter
static HRESULT STDMETHODCALLTYPE idroptarget_dragenter(IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	if(ghwnd!=0){
		RECT rect={0};
		char msg[100]={0};
		GetWindowRect(ghwnd,&rect);
		get_correct_msg(msg,sizeof(msg));
		create_tooltip(ghwnd,msg,rect.left,rect.top);
	}
	return S_OK;
}

//	IDropTarget::DragOver
static HRESULT STDMETHODCALLTYPE idroptarget_dragover(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	char msg[100]={0};
	get_correct_msg(msg,sizeof(msg));
	update_tooltip_text(msg);
	return S_OK;
}

//	IDropTarget::DragLeave
static HRESULT STDMETHODCALLTYPE idroptarget_dragleave(IDropTarget* This)
{
	destroy_tooltip();
	return S_OK;
}

//	IDropTarget::Drop
static HRESULT STDMETHODCALLTYPE idroptarget_drop(IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	FORMATETC fmtetc={CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM stg;
	//printf("drag drop\n");
	if(S_OK==pDataObject->lpVtbl->GetData(pDataObject,&fmtetc,&stg)){
			process_drop(ghwnd,(HANDLE)stg.hGlobal,GetKeyState(VK_CONTROL)&0x8000,
				GetKeyState(VK_SHIFT)&0x8000);

	}
	destroy_tooltip();
	return S_OK;
}
static IDropTargetVtbl idt_vtbl={
	idroptarget_queryinterface,
	idroptarget_addref,
	idroptarget_release,
	idroptarget_dragenter,
	idroptarget_dragover,
	idroptarget_dragleave,
	idroptarget_drop
};
int register_drag_drop(HWND hwnd)
{
	ghwnd=hwnd;
	droptarget.lpVtbl=(IDropTargetVtbl*)&idt_vtbl;
	return RegisterDragDrop(hwnd,&droptarget);
}