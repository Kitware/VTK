/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDIView.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "stdafx.h"
#include "vtkMDI.h"

#include "vtkMDIDoc.h"
#include "vtkMDIView.h"
#include "vtkTextProperty.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView

IMPLEMENT_DYNCREATE(CVtkMDIView, CView)

BEGIN_MESSAGE_MAP(CVtkMDIView, CView)
  //{{AFX_MSG_MAP(CVtkMDIView)
  ON_WM_SIZE()
  ON_WM_ERASEBKGND()
  ON_WM_CREATE()
  //}}AFX_MSG_MAP
  // Standard printing commands
  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView construction/destruction

CVtkMDIView::CVtkMDIView()
{
  // Create the the renderer, window and interactor objects.
  this->ren = vtkRenderer::New();
  this->renWin = vtkWin32OpenGLRenderWindow::New();
  this->iren = vtkWin32RenderWindowInteractor::New();
  
  // Create the the objects used to form the visualisation.
  this->Mapper = vtkDataSetMapper::New();
  this->Actor = vtkActor::New();

  this->txtActor = vtkActor2D::New();
  this->txtMapper = vtkTextMapper::New();

}

CVtkMDIView::~CVtkMDIView()
{
  // Delete the the renderer, window and interactor objects.
    this->ren->Delete();
    this->iren->Delete();
    this->renWin->Delete();

  // Delete the the objects used to form the visualisation.
  this->Mapper->Delete();
  this->Actor->Delete();

  this->txtActor->Delete();
  this->txtMapper->Delete();
}

BOOL CVtkMDIView::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView drawing

/*!
 * Based on a comment from Klaus Nowikow in vtkusers list. 
 * <http://public.kitware.com/pipermail/vtkusers/1999-August/001852.html>
 *
 * Here we intialise the interactor and set up for printing and displaying.
 *
 * @param pDC : 
 *
 * @return void  : 
 */
void CVtkMDIView::OnDraw(CDC* pDC)
{
  CVtkMDIDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

  if ( !this->iren->GetInitialized() )
  {
    CRect rect;

    this->GetClientRect(&rect);
    this->iren->Initialize();
    this->renWin->SetSize(rect.right-rect.left,rect.bottom-rect.top);

    this->ren->ResetCamera();

  }

  // Invoke the pipeline
  Pipeline();

  if ( pDC->IsPrinting() )
  {
    this->BeginWaitCursor();

    // Obtain the size of the printer page in pixels.
    int cxPage = pDC->GetDeviceCaps(HORZRES);
    int cyPage = pDC->GetDeviceCaps(VERTRES);

    // Get the size of the window in pixels.
    int *size = this->renWin->GetSize();
    int cxWindow = size[0];
    int cyWindow = size[1];
    float fx = float(cxPage) / float(cxWindow);
    float fy = float(cyPage) / float(cyWindow);
    float scale = min(fx,fy);
    int x = int(scale * float(cxWindow));
    int y = int(scale * float(cyWindow));
    this->renWin->SetupMemoryRendering(cxWindow, cyWindow, pDC->GetSafeHdc());
    this->renWin->Render();
    HDC memDC = this->renWin->GetMemoryDC();
    StretchBlt(pDC->GetSafeHdc(),0,0,x,y,memDC,0,0,cxWindow,cyWindow,SRCCOPY);
    this->renWin->ResumeScreenRendering();

    this->EndWaitCursor();

  }
  else
  {
    this->renWin->Render();
  }
}

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView printing

BOOL CVtkMDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CVtkMDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void CVtkMDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView diagnostics

#ifdef _DEBUG
void CVtkMDIView::AssertValid() const
{
  CView::AssertValid();
}

void CVtkMDIView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CVtkMDIDoc* CVtkMDIView::GetDocument() // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVtkMDIDoc)));
  return (CVtkMDIDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIView message handlers

/*!
 * Resize the render window to that of the view window.
 *
 * @param nType : 
 * @param cx : 
 * @param cy : 
 *
 * @return void  : 
 */
void CVtkMDIView::OnSize(UINT nType, int cx, int cy) 
{
  CView::OnSize(nType, cx, cy);
  
  CRect rect;
  this->GetClientRect(&rect);
  this->renWin->SetSize(rect.right-rect.left,rect.bottom-rect.top);
  
}

/*! 
 * Based on a comment from Nigel Nunn in vtkusers list. 
 * <http://public.kitware.com/pipermail/vtkusers/2001-May/006371.html>
 *
 * Overriding OnEraseBkgnd() stops that horrible 
 * flickering on resizing.  The Renderer likes to do 
 * such things itself! 
 *
 * @param pDC : 
 *
 * @return BOOL  : 
 */
BOOL CVtkMDIView::OnEraseBkgnd(CDC* pDC) 
{
  return TRUE;
}

/*!
 * Add the renderer to the vtk window and link the wiew to the vtk window.
 * We assume that the objects have been already 
 * created in the constructor.
 *
 * @param lpCreateStruct : 
 *
 * @return int  : 
 */
int CVtkMDIView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;
  
  this->renWin->AddRenderer(this->ren);
  // setup the parent window
  this->renWin->SetParentId(this->m_hWnd);
  this->iren->SetRenderWindow(this->renWin);
    
  return 0;
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
LRESULT CVtkMDIView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  switch (message)
  {
    //case WM_PAINT: 
    case WM_LBUTTONDOWN: 
    case WM_LBUTTONUP: 
    case WM_MBUTTONDOWN: 
    case WM_MBUTTONUP: 
    case WM_RBUTTONDOWN: 
    case WM_RBUTTONUP: 
    case WM_MOUSEMOVE:
    case WM_CHAR:
    case WM_TIMER:
      if (this->iren->GetInitialized())
      {
        return vtkHandleMessage2(this->m_hWnd, message, wParam, lParam, this->iren);
      }
      break;
  }
   
  return CView::WindowProc(message, wParam, lParam);
}


/*!
 * This is the pipeline that creates the object for 
 * rendering on the screen.
 * Here is where we create the pipeline.
 * We assume that the objects have been already 
 * created in the constructor and that the 
 * renderer, window and interactor have been already
 * initialised in OnCreate()
 *
 * @param none
 *
 * @return void  : 
 */
void CVtkMDIView::Pipeline()
{
  if ( this->GetDocument()->HasAFile )
  {
    this->Mapper->SetInput(this->GetDocument()->Reader->GetOutput());
    this->Actor->SetMapper(this->Mapper);

    this->txtMapper->SetInput(this->GetDocument()->Reader->GetFileName());
    this->txtMapper->GetTextProperty()->SetFontSize(12);
    this->txtActor->SetMapper(this->txtMapper);

    this->ren->SetBackground(0.2,0.5,0.3);
    this->ren->AddActor(this->Actor);
    this->ren->AddActor(this->txtActor);
  }
  else
  {
    this->txtMapper->SetInput("Hello World");
    this->txtMapper->GetTextProperty()->SetFontSize(24);
    this->txtActor->SetMapper(this->txtMapper);

    this->ren->SetBackground(0.2,0.5,0.3);
    this->ren->AddActor(this->txtActor);
  }
}

