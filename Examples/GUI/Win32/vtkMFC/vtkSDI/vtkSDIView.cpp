/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSDIView.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "stdafx.h"
#include "vtkSDI.h"

#include "vtkSDIDoc.h"
#include "vtkSDIView.h"
#include "vtkPolyData.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIView

IMPLEMENT_DYNCREATE(CVtkSDIView, CView)

BEGIN_MESSAGE_MAP(CVtkSDIView, CView)
  //{{AFX_MSG_MAP(CVtkSDIView)
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
// CVtkSDIView construction/destruction

CVtkSDIView::CVtkSDIView()
{
  // Create the the renderer, window and interactor objects.
  this->ren = vtkRenderer::New();
  this->renWin = vtkWin32OpenGLRenderWindow::New();
  this->iren = vtkWin32RenderWindowInteractor::New();
  
  // Create the the objects used to form the visualisation.
  this->sphere = vtkSphereSource::New();
  this->sphereElevation = vtkElevationFilter::New();
  this->sphereMapper = vtkPolyDataMapper::New();
  this->sphereActor = vtkActor::New();
  this->cone = vtkConeSource::New();
  this->glyph = vtkGlyph3D::New();
  this->spikeMapper = vtkPolyDataMapper::New();
  this->spikeActor = vtkActor::New();
  this->sphereAxis = vtkCubeAxesActor2D::New();

}

CVtkSDIView::~CVtkSDIView()
{
  // Delete the the renderer, window and interactor objects.
    this->ren->Delete();
    this->iren->Delete();
    this->renWin->Delete();

  // Delete the the objects used to form the visualisation.
  this->sphere->Delete();
  this->sphereElevation->Delete();
  this->sphereMapper->Delete();
  this->sphereActor->Delete();
  this->cone->Delete();
  this->glyph->Delete();
  this->spikeMapper->Delete();
  this->spikeActor->Delete();
  this->sphereAxis->Delete();
}

BOOL CVtkSDIView::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs

  return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIView drawing


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
void CVtkSDIView::OnDraw(CDC* pDC)
{
  CVtkSDIDoc* pDoc = GetDocument();
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
// CVtkSDIView printing

BOOL CVtkSDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CVtkSDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void CVtkSDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIView diagnostics

#ifdef _DEBUG
void CVtkSDIView::AssertValid() const
{
  CView::AssertValid();
}

void CVtkSDIView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CVtkSDIDoc* CVtkSDIView::GetDocument() // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVtkSDIDoc)));
  return (CVtkSDIDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIView message handlers

/*!
 * Resize the render window to that of the view window.
 *
 * @param nType : 
 * @param cx : 
 * @param cy : 
 *
 * @return void  : 
 */
void CVtkSDIView::OnSize(UINT nType, int cx, int cy) 
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
BOOL CVtkSDIView::OnEraseBkgnd(CDC* pDC) 
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
int CVtkSDIView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
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
LRESULT CVtkSDIView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
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
void CVtkSDIView::Pipeline()
{
  // Construct the sphere
  this->sphere->SetRadius(1);
  this->sphere->SetThetaResolution(18);
  this->sphere->SetPhiResolution(18);
  this->sphere->LatLongTessellationOn();
  // Generate elevations
  this->sphereElevation->SetInput(this->sphere->GetOutput());
  this->sphereElevation->SetLowPoint(0,0,-1);
  this->sphereElevation->SetHighPoint(0,0,1);
  this->sphereElevation->SetScalarRange(-1,1);
  // Link the mapper
  this->sphereMapper->SetInput(this->sphereElevation->GetPolyDataOutput());
  this->sphereMapper->SetColorModeToMapScalars();
  this->sphereMapper->SetScalarRange(-1,1);
  // Link the actor
  this->sphereActor->SetMapper(this->sphereMapper);
  // Add it to the renderer
  this->ren->AddActor(this->sphereActor);

  // Construct the cone
  this->cone->SetResolution(8);

  // Construct the glyphs on the spherical surface
  this->glyph->SetInput(this->sphere->GetOutput());
  this->glyph->SetSource(this->cone->GetOutput());
  this->glyph->SetVectorModeToUseNormal();
  this->glyph->SetScaleModeToScaleByVector();
  this->glyph->SetScaleFactor(0.1);
  // Link the mapper to the glyph
  this->spikeMapper->SetInput(this->glyph->GetOutput());
  // Link the actor
  this->spikeActor->SetMapper(this->spikeMapper);
  // Add it to the renderer
  this->ren->AddActor(this->spikeActor);

  // Add in the cube axis actor
  this->sphereAxis->SetInput(this->sphereElevation->GetOutput());
  this->sphereAxis->SetCamera(this->ren->GetActiveCamera());
  // Add it to the renderer
  this->ren->AddActor(this->sphereAxis);
}




