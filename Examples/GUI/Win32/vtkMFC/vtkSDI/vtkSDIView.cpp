// vtkSDIView.cpp : implementation of the CvtkSDIView class
//

#include "stdafx.h"
#include "vtkSDI.h"

#include "vtkSDIDoc.h"
#include "vtkSDIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CvtkSDIView

IMPLEMENT_DYNCREATE(CvtkSDIView, CView)

BEGIN_MESSAGE_MAP(CvtkSDIView, CView)
  // Standard printing commands
  ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_ERASEBKGND()
  ON_WM_SIZE()
  ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

// CvtkSDIView construction/destruction

CvtkSDIView::CvtkSDIView()
{
  this->pvtkMFCWindow     = NULL;

  // Create the the renderer, window and interactor objects.
  this->pvtkRenderer    = vtkRenderer::New();
}

CvtkSDIView::~CvtkSDIView()
{
  // delete generic vtk window
  if (this->pvtkMFCWindow) delete this->pvtkMFCWindow;
}

void CvtkSDIView::OnDraw(CDC* pDC)
{
  CvtkSDIDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

  if (this->pvtkMFCWindow)
  {
    if (pDC->IsPrinting())
      this->pvtkMFCWindow->DrawDC(pDC);
  }
}


// CvtkSDIView printing

BOOL CvtkSDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CvtkSDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void CvtkSDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}


// CvtkSDIView diagnostics

#ifdef _DEBUG
void CvtkSDIView::AssertValid() const
{
  CView::AssertValid();
}

void CvtkSDIView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CvtkSDIDoc* CvtkSDIView::GetDocument() const // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CvtkSDIDoc)));
  return (CvtkSDIDoc*)m_pDocument;
}
#endif //_DEBUG


// CvtkSDIView message handlers

int CvtkSDIView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;

  return 0;
}

void CvtkSDIView::OnDestroy()
{
  // Delete the the renderer, window and interactor objects.
  if (this->pvtkRenderer)      this->pvtkRenderer->Delete();

  // destroy base
  CView::OnDestroy();
}

BOOL CvtkSDIView::OnEraseBkgnd(CDC* pDC)
{
  return TRUE;
}

void CvtkSDIView::OnSize(UINT nType, int cx, int cy)
{
  CView::OnSize(nType, cx, cy);

  if (this->pvtkMFCWindow)
    this->pvtkMFCWindow->MoveWindow(0, 0, cx, cy);
}

void CvtkSDIView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  AfxMessageBox("You made a double click");

  CView::OnLButtonDblClk(nFlags, point);
}

void CvtkSDIView::OnInitialUpdate()
{
  CView::OnInitialUpdate();

  if (this->pvtkMFCWindow) delete this->pvtkMFCWindow;
  this->pvtkMFCWindow = new vtkMFCWindow(this);

  this->pvtkMFCWindow->GetRenderWindow()->AddRenderer(this->pvtkRenderer);
}
