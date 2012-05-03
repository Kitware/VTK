/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCWindow.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#ifndef WINVER
#define WINVER 0x0400
#endif

// Define _WIN32_WINNT and _WIN32_IE to avoid the following error with Visual
// Studio 2008 SP1:
// "C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\sdkddkver.h(217) :
// fatal error C1189: #error :  _WIN32_WINNT settings conflicts with _WIN32_IE
// setting"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400 // =_WIN32_WINNT_NT4
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0601 //=_WIN32_IE_IE60SP1
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

  ON_WM_LBUTTONDBLCLK()
  ON_WM_LBUTTONDOWN()
  ON_WM_MBUTTONDOWN()
  ON_WM_RBUTTONDOWN()
  ON_WM_LBUTTONUP()
  ON_WM_MBUTTONUP()
  ON_WM_RBUTTONUP()
  ON_WM_MOUSEMOVE()
  ON_WM_MOUSEWHEEL()
  ON_WM_CHAR()
  ON_WM_KEYUP()
  ON_WM_KEYDOWN()
  ON_WM_TIMER()

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

void vtkMFCWindow::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnLButtonDown(this->GetSafeHwnd(), nFlags, point.x, point.y, 1);
}

void vtkMFCWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
  this->SetFocus();
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnLButtonDown(this->GetSafeHwnd(), nFlags, point.x, point.y, 0);
}

void vtkMFCWindow::OnMButtonDown(UINT nFlags, CPoint point)
{
  this->SetFocus();
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnMButtonDown(this->GetSafeHwnd(), nFlags, point.x, point.y, 0);
}

void vtkMFCWindow::OnRButtonDown(UINT nFlags, CPoint point)
{
  this->SetFocus();
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnRButtonDown(this->GetSafeHwnd(), nFlags, point.x, point.y, 0);
}

void vtkMFCWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnLButtonUp(this->GetSafeHwnd(), nFlags, point.x, point.y);
}

void vtkMFCWindow::OnMButtonUp(UINT nFlags, CPoint point)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnMButtonUp(this->GetSafeHwnd(), nFlags, point.x, point.y);
}

void vtkMFCWindow::OnRButtonUp(UINT nFlags, CPoint point)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnRButtonUp(this->GetSafeHwnd(), nFlags, point.x, point.y);
}

void vtkMFCWindow::OnMouseMove(UINT nFlags, CPoint point)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnMouseMove(this->GetSafeHwnd(), nFlags, point.x, point.y);
}

BOOL vtkMFCWindow::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
  if(zDelta > 0)
    static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
      OnMouseWheelForward(this->GetSafeHwnd(), nFlags, point.x, point.y);
  else
    static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
      OnMouseWheelBackward(this->GetSafeHwnd(), nFlags, point.x, point.y);
  return TRUE;
}

void vtkMFCWindow::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnChar(this->GetSafeHwnd(), nChar, nRepCnt, nFlags);
}

void vtkMFCWindow::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnKeyUp(this->GetSafeHwnd(), nChar, nRepCnt, nFlags);
}

void vtkMFCWindow::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnKeyDown(this->GetSafeHwnd(), nChar, nRepCnt, nFlags);
}

void vtkMFCWindow::OnTimer(UINT_PTR nIDEvent)
{
  static_cast<vtkWin32RenderWindowInteractor*>(this->GetInteractor())->
    OnTimer(this->GetSafeHwnd(), (UINT) nIDEvent);
}
