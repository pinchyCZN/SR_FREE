#include <windows.h>
#include <oleidl.h>
static 	IDropTarget droptarget;
static HWND ghwnd=0;
static char original_title_bar[80]={0};

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
	printf("dragenter\n");
	if(ghwnd!=0){
		original_title_bar[0]=0;
		GetWindowText(ghwnd,original_title_bar,sizeof(original_title_bar));
		original_title_bar[sizeof(original_title_bar)-1]=0;
		SetWindowText(ghwnd,"CTRL=mask exact filename SHIFT");
	}
	return S_OK;
}

//	IDropTarget::DragOver
static HRESULT STDMETHODCALLTYPE idroptarget_dragover(IDropTarget* This, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	printf("dragover\n");
	return S_OK;
}

//	IDropTarget::DragLeave
static HRESULT STDMETHODCALLTYPE idroptarget_dragleave(IDropTarget* This)
{
	printf("drag leave\n");
	if(ghwnd!=0 && original_title_bar[0]!=0){
		SetWindowText(ghwnd,original_title_bar);
	}
	return S_OK;
}

//	IDropTarget::Drop
static HRESULT STDMETHODCALLTYPE idroptarget_drop(IDropTarget* This, IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	printf("drag drop\n");
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