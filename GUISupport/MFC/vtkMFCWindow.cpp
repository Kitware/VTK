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

#include "vtkMFCWindow.h"

#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"
#include "vtkInteractorStyleSwitch.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView

IMPLEMENT_DYNCREATE(vtkMFCWindow, CWnd)

BEGIN_MESSAGE_MAP(vtkMFCWindow, CWnd)
  //{{AFX_MSG_MAP(vtkMFCWindow)
  ON_WM_SIZE()
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_CREATE()
  ON_WM_DESTROY()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView construction/destruction

vtkMFCWindow::vtkMFCWindow()
{
  this->renWin = vtkWin32OpenGLRenderWindow::New();
  this->iren = vtkWin32RenderWindowInteractor::New();
  this->iren->SetInstallMessageProc(0);
}

vtkMFCWindow::~vtkMFCWindow()
{
  iren->Delete();
  renWin->Delete();
}

BOOL vtkMFCWindow::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CWnd::PreCreateWindow(cs);
}

/* finalize the window when we destroy */
void vtkMFCWindow::OnDestroy()
{
  this->renWin->Finalize();
}

/*!
 * connect window to vtk window
 */
int vtkMFCWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CWnd::OnCreate(lpCreateStruct) == -1)
    return -1;
  
  // setup the parent window
  this->renWin->SetWindowId(this->GetSafeHwnd());
  this->iren->SetRenderWindow(this->renWin);
  this->iren->Initialize();


  // make a default interactor style if one doesn't exist
  if(!this->iren->GetInteractorStyle())
    {
    vtkInteractorStyle* style = vtkInteractorStyleSwitch::New();
    iren->SetInteractorStyle(style);
    style->Delete();
    }

  return 0;
}


vtkWin32OpenGLRenderWindow* vtkMFCWindow::GetRenderWindow()
{
  return this->renWin;
}


/////////////////////////////////////////////////////////////////////////////
// vtkMFCWindow drawing

/*!
 * Based on a comment from Klaus Nowikow in vtkusers list. 
 * <http://public.kitware.com/pipermail/vtkusers/1999-August/001852.html>
 *
 *
 * @return void  : 
 */

void vtkMFCWindow::OnPaint()
{
  CPaintDC dc(this);
  this->renWin->Render();
}

/////////////////////////////////////////////////////////////////////////////
// vtkMFCWindow message handlers

/*!
 * Resize the render window to that of the view window.
 *
 * @param nType : 
 * @param cx : 
 * @param cy : 
 *
 * @return void  : 
 */
void vtkMFCWindow::OnSize(UINT nType, int cx, int cy) 
{
  CWnd::OnSize(nType, cx, cy);

  this->renWin->vtkRenderWindow::SetSize(cx,cy);
  this->iren->SetSize(cx,cy);
}

BOOL vtkMFCWindow::OnEraseBkgnd(CDC*) 
{
  // don't erase background
  return TRUE;
}


/*!
 * Allow the interactor to interact with the keyboard.
 *
 * @param message : 
 * @param wParam : 
 * @param lParam : 
 *
 * @return LRESULT  : 
 */
LRESULT vtkMFCWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{

  switch (message)
  {
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

        
      if (this->iren->GetInitialized())
      {
        return vtkHandleMessage2(this->m_hWnd, message, wParam, lParam, this->iren);
      }
      break;

  }
   
  return CWnd::WindowProc(message, wParam, lParam);
}





/////////////////////////////////////////////////////////////////////////////
// vtkMFCView diagnostics

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

