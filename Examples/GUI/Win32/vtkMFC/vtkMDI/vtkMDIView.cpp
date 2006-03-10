#include "stdafx.h"
#include "vtkMDI.h"

#include "vtkMDIDoc.h"
#include "vtkMDIView.h"

#include "vtkCallbackCommand.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CvtkMDIView, CView)

BEGIN_MESSAGE_MAP(CvtkMDIView, CView)
  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
  ON_WM_DESTROY()
  ON_WM_CREATE()
  ON_WM_LBUTTONDBLCLK()
  ON_WM_SIZE()
  ON_WM_ERASEBKGND()
  ON_WM_SIZE()
END_MESSAGE_MAP()


CvtkMDIView::CvtkMDIView()
{
  this->pvtkMFCWindow     = NULL;

  // Create the the renderer, window and interactor objects.
  this->pvtkRenderer    = vtkRenderer::New();

  // Create the the objects used to form the visualisation.
  this->pvtkDataSetMapper  = vtkDataSetMapper::New();
  this->pvtkActor      = vtkActor::New();
  this->pvtkActor2D    = vtkActor2D::New();
  this->pvtkTextMapper  = vtkTextMapper::New();
}

CvtkMDIView::~CvtkMDIView()
{
  // delete generic vtk window
  if (this->pvtkMFCWindow) delete this->pvtkMFCWindow;
}

// CvtkMDIView drawing
void CvtkMDIView::OnDraw(CDC* pDC)
{
  CvtkMDIDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

  if (this->pvtkMFCWindow)
  {
    if (pDC->IsPrinting())
      this->pvtkMFCWindow->DrawDC(pDC);
  }
}


// CvtkMDIView printing

BOOL CvtkMDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CvtkMDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void CvtkMDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}


// CvtkMDIView diagnostics

#ifdef _DEBUG
void CvtkMDIView::AssertValid() const
{
  CView::AssertValid();
}

void CvtkMDIView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CvtkMDIDoc* CvtkMDIView::GetDocument() const // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CvtkMDIDoc)));
  return (CvtkMDIDoc*)m_pDocument;
}
#endif //_DEBUG


// CvtkMDIView message handlers

void CvtkMDIView::ExecutePipeline()
{
  CvtkMDIDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  if (pDoc->pvtkDataSetReader)  // have file
  {
    this->pvtkDataSetMapper->SetInput(this->GetDocument()->pvtkDataSetReader->GetOutput());
    this->pvtkActor->SetMapper(this->pvtkDataSetMapper);

    this->pvtkTextMapper->SetInput(this->GetDocument()->pvtkDataSetReader->GetFileName());
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(12);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    this->pvtkRenderer->SetBackground(0.0,0.0,0.4);
    this->pvtkRenderer->AddActor(this->pvtkActor);
    this->pvtkRenderer->AddActor(this->pvtkActor2D);
  }
  else  // have no file
  {
    this->pvtkTextMapper->SetInput("Hello World");
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(24);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    this->pvtkRenderer->SetBackground(0.0,0.0,0.4);
    this->pvtkRenderer->AddActor(this->pvtkActor2D);
  }
}

static void handle_double_click(vtkObject* obj, unsigned long,
                                void*, void*)
{
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  if(iren && iren->GetRepeatCount())
    {
    AfxMessageBox("Double Click");
    }
}

void CvtkMDIView::OnInitialUpdate()
{
  CView::OnInitialUpdate();

  this->pvtkMFCWindow = new vtkMFCWindow(this);

  this->pvtkMFCWindow->GetRenderWindow()->AddRenderer(this->pvtkRenderer);

    // get double click events
  vtkCallbackCommand* callback = vtkCallbackCommand::New();
  callback->SetCallback(handle_double_click);
  this->pvtkMFCWindow->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, callback, 1.0);
  callback->Delete();

  // execute object pipeline
  ExecutePipeline();
}

void CvtkMDIView::OnDestroy()
{
  // Delete the the renderer, window and interactor objects.
  if (this->pvtkRenderer)      this->pvtkRenderer->Delete();

  // Delete the the objects used to form the visualisation.
  if (this->pvtkDataSetMapper)  this->pvtkDataSetMapper->Delete();
  if (this->pvtkActor)      this->pvtkActor->Delete();
  if (this->pvtkActor2D)      this->pvtkActor2D->Delete();
  if (this->pvtkTextMapper)    this->pvtkTextMapper->Delete();

  // destroy base
  CView::OnDestroy();
}

int CvtkMDIView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;

  return 0;
}

BOOL CvtkMDIView::OnEraseBkgnd(CDC*) 
{
  return TRUE;
}

void CvtkMDIView::OnSize(UINT nType, int cx, int cy) 
{
  CView::OnSize(nType, cx, cy);

  if (this->pvtkMFCWindow)
    this->pvtkMFCWindow->MoveWindow(0, 0, cx, cy);
}

