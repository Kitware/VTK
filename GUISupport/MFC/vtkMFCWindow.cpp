

#ifndef WINVER    // Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400
#endif

#include "vtkMFCWindow.h"

#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(vtkMFCWindow, CWnd)
  ON_WM_SIZE()
  ON_WM_PAINT()
  ON_WM_DESTROY()
  ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

#ifdef _DEBUG
void vtkMFCWindow::AssertValid() const
{
  CWnd::AssertValid();
}

void vtkMFCWindow::Dump(CDumpContext& dc) const
{
  CWnd::Dump(dc);
}
#endif //_DEBUG


vtkMFCWindow::vtkMFCWindow(CWnd *pcWnd)
{
  this->pvtkWin32OpenGLRW = NULL;

  // create self as a child of passed in parent
  DWORD style = WS_VISIBLE | WS_CLIPSIBLINGS;
  if(pcWnd)
    style |= WS_CHILD;
  BOOL bCreated = CWnd::Create(NULL, _T("VTK-MFC Window"),
                               style, CRect(0, 0, 1, 1), 
                               pcWnd, (UINT)IDC_STATIC);

  SUCCEEDED(bCreated);

  // create a default vtk window
  vtkWin32OpenGLRenderWindow* win = vtkWin32OpenGLRenderWindow::New();
  this->SetRenderWindow(win);
  win->Delete();

}

vtkMFCWindow::~vtkMFCWindow()
{
  this->SetRenderWindow(NULL);
}

void vtkMFCWindow::OnDestroy()
{
  if(this->pvtkWin32OpenGLRW && this->pvtkWin32OpenGLRW->GetMapped())
    this->pvtkWin32OpenGLRW->Finalize();

  CWnd::OnDestroy();
}

void vtkMFCWindow::SetRenderWindow(vtkWin32OpenGLRenderWindow* win)
{

  if(this->pvtkWin32OpenGLRW)
    {
    if(this->pvtkWin32OpenGLRW->GetMapped())
      this->pvtkWin32OpenGLRW->Finalize();
    this->pvtkWin32OpenGLRW->UnRegister(NULL);
    }

  this->pvtkWin32OpenGLRW = win;

  if(this->pvtkWin32OpenGLRW)
    {
    this->pvtkWin32OpenGLRW->Register(NULL);

    vtkWin32RenderWindowInteractor* iren = vtkWin32RenderWindowInteractor::New();
    iren->SetInstallMessageProc(0);

    // setup the parent window
    this->pvtkWin32OpenGLRW->SetWindowId(this->GetSafeHwnd());
    this->pvtkWin32OpenGLRW->SetParentId(::GetParent(this->GetSafeHwnd()));
    iren->SetRenderWindow(this->pvtkWin32OpenGLRW);

    iren->Initialize();

    // update size
    CRect cRect = CRect(0,0,1,1);
    if(this->GetParent())
      this->GetParent()->GetClientRect(&cRect);
    if (iren->GetInitialized())
      iren->UpdateSize(cRect.Width(), cRect.Height());

    // release our hold on interactor
    iren->Delete();
    }
}

vtkWin32OpenGLRenderWindow* vtkMFCWindow::GetRenderWindow()
{
  return this->pvtkWin32OpenGLRW;
}

vtkRenderWindowInteractor* vtkMFCWindow::GetInteractor()
{
  if(!this->pvtkWin32OpenGLRW)
    return NULL;
  return this->pvtkWin32OpenGLRW->GetInteractor();
}

void vtkMFCWindow::OnPaint()
{
  CPaintDC dc(this);
  if (this->GetInteractor() && this->GetInteractor()->GetInitialized())
    {
    this->GetInteractor()->Render();
    }
}

void vtkMFCWindow::DrawDC(CDC* pDC)
{
  // Obtain the size of the printer page in pixels.
  int cxPage = pDC->GetDeviceCaps(HORZRES);
  int cyPage = pDC->GetDeviceCaps(VERTRES);

  // Get the size of the window in pixels.
  int *size = this->pvtkWin32OpenGLRW->GetSize();
  int cxWindow = size[0];
  int cyWindow = size[1];
  float fx = float(cxPage) / float(cxWindow);
  float fy = float(cyPage) / float(cyWindow);
  float scale = min(fx,fy);
  int x = int(scale * float(cxWindow));
  int y = int(scale * float(cyWindow));
  this->pvtkWin32OpenGLRW->SetupMemoryRendering(cxWindow, cyWindow, pDC->GetSafeHdc());
  this->pvtkWin32OpenGLRW->Render();
  HDC memDC = this->pvtkWin32OpenGLRW->GetMemoryDC();
  StretchBlt(pDC->GetSafeHdc(),0,0,x,y,memDC,0,0,cxWindow,cyWindow,SRCCOPY);
  this->pvtkWin32OpenGLRW->ResumeScreenRendering();
}

void vtkMFCWindow::OnSize(UINT nType, int cx, int cy) 
{
  CWnd::OnSize(nType, cx, cy);
  if (this->GetInteractor() && this->GetInteractor()->GetInitialized())
    this->GetInteractor()->UpdateSize(cx, cy);
}

BOOL vtkMFCWindow::OnEraseBkgnd(CDC*) 
{
  return TRUE;
}


LRESULT vtkMFCWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  switch (message)
  {
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_LBUTTONDOWN: 
  case WM_MBUTTONDOWN: 
  case WM_RBUTTONDOWN: 
    // grab focus on mouse downs
    this->SetFocus();
    // follow through

  case WM_MBUTTONUP: 
  case WM_RBUTTONUP: 
  case WM_LBUTTONUP: 
  case WM_MOUSEMOVE:
  case WM_MOUSEWHEEL:
  case WM_CHAR:
  case WM_KEYDOWN:
  case WM_KEYUP:
  case WM_TIMER:

    if (this->GetInteractor()->GetInitialized())
      {
      return vtkHandleMessage2(this->m_hWnd, message, wParam, lParam, 
        static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor()));
      }
    break;

  }

  return CWnd::WindowProc(message, wParam, lParam);
}

