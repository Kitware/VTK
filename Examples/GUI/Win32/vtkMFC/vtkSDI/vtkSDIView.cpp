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

#include "vtkMFCWindow.h"

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
  ON_WM_CREATE()
  ON_WM_ERASEBKGND()
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

  this->vtkWindow = new vtkMFCWindow;
  
  this->ren = vtkRenderer::New();


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
  delete this->vtkWindow;

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
  CView::OnDraw(pDC);

  CVtkSDIDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);


  // Invoke the pipeline
  Pipeline();

  if ( pDC->IsPrinting() )
  {
    this->BeginWaitCursor();

    // Obtain the size of the printer page in pixels.
    int cxPage = pDC->GetDeviceCaps(HORZRES);
    int cyPage = pDC->GetDeviceCaps(VERTRES);

    // Get the size of the window in pixels.
    int *size = this->vtkWindow->GetRenderWindow()->GetSize();
    int cxWindow = size[0];
    int cyWindow = size[1];
    float fx = float(cxPage) / float(cxWindow);
    float fy = float(cyPage) / float(cyWindow);
    float scale = min(fx,fy);
    int x = int(scale * float(cxWindow));
    int y = int(scale * float(cyWindow));
    this->vtkWindow->GetRenderWindow()->SetupMemoryRendering(cxWindow, cyWindow, pDC->GetSafeHdc());
    this->vtkWindow->GetRenderWindow()->Render();
    HDC memDC = this->vtkWindow->GetRenderWindow()->GetMemoryDC();
    StretchBlt(pDC->GetSafeHdc(),0,0,x,y,memDC,0,0,cxWindow,cyWindow,SRCCOPY);
    this->vtkWindow->GetRenderWindow()->ResumeScreenRendering();

    this->EndWaitCursor();

  }

}

BOOL CVtkSDIView::OnEraseBkgnd(CDC*) 
{
  return TRUE;
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

  // vtk window takes up entire client view
  RECT rect;
  this->GetClientRect(&rect);
  this->vtkWindow->MoveWindow(&rect);
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
  

  this->vtkWindow->GetRenderWindow()->AddRenderer(this->ren);

  // setup the vtk window
  this->vtkWindow->Create(0, _T("VTK window"), 
                          WS_CHILD | WS_VISIBLE, 
                          CRect(), this, 0);
  
  return 0;
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




