// vtkMFCRenderView.cpp : implementation file
//

#include "stdafx.h"
#include "vtkMFCRenderView.h"

#include "vtkMFCDocument.h"

#include "vtkRenderer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// vtkMFCRenderView

IMPLEMENT_DYNCREATE(vtkMFCRenderView, vtkMFCView)

vtkMFCRenderView::vtkMFCRenderView()
{
  this->Renderer = vtkRenderer::New();
  this->RenderWindow = vtkWin32OpenGLRenderWindow::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkWin32RenderWindowInteractor::New();
}

vtkMFCRenderView::~vtkMFCRenderView()
{
  if (this->Interactor)
    {
    this->Interactor->Delete();
    }
  if (this->Renderer)
    {
    this->Renderer->SetRenderWindow(NULL);
    }
  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    }
  if (this->Renderer)
    {
    this->Renderer->Delete();
    }
}


BEGIN_MESSAGE_MAP(vtkMFCRenderView, vtkMFCView)
  //{{AFX_MSG_MAP(vtkMFCRenderView)
  ON_WM_CREATE()
  ON_WM_SIZE()
  //}}AFX_MSG_MAP
  ON_COMMAND(ID_FILE_PRINT, vtkMFCRenderView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, vtkMFCRenderView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, vtkMFCRenderView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// vtkMFCRenderView drawing

void vtkMFCRenderView::OnDraw(CDC* pDC)
{
  CDocument* pDoc = GetDocument();
  ASSERT_VALID(pDoc);

  if (!this->Interactor->GetInitialized())
    {
    this->Interactor->SetRenderWindow(this->RenderWindow);
    WNDPROC OldProc = (WNDPROC)vtkGetWindowLong(this->m_hWnd,vtkGWL_WNDPROC);
    this->Interactor->Initialize();
    vtkSetWindowLong(this->m_hWnd,vtkGWL_WNDPROC,(vtkLONG)OldProc);
    }

  // TODO: add draw code for native data here
  if (pDC->IsPrinting())
    {
    int size[2];
    float scale;

    BeginWaitCursor();
    memcpy(size,this->RenderWindow->GetSize(),sizeof(int)*2);

    int cxDIB = size[0];         // Size of DIB - x
    int cyDIB = size[1];         // Size of DIB - y
    CRect rcDest;

    // get size of printer page (in pixels)
    int cxPage = pDC->GetDeviceCaps(HORZRES);
    int cyPage = pDC->GetDeviceCaps(VERTRES);
    // get printer pixels per inch
    int cxInch = pDC->GetDeviceCaps(LOGPIXELSX);
    int cyInch = pDC->GetDeviceCaps(LOGPIXELSY);
    scale = cxInch/this->GetPrintDPI();

    //
    // Best Fit case -- create a rectangle which preserves
    // the DIB's aspect ratio, and fills the page horizontally.
    //
    // The formula in the "->bottom" field below calculates the Y
    // position of the printed bitmap, based on the size of the
    // bitmap, the width of the page, and the relative size of
    // a printed pixel (cyInch / cxInch).
    //
    rcDest.bottom = rcDest.left = 0;
    if (((float)cyDIB*(float)cxPage/(float)cxInch) >
        ((float)cxDIB*(float)cyPage/(float)cyInch))
      {
      rcDest.top = cyPage;
      rcDest.right = ((float)(cyPage*cxInch*cxDIB)) /
        ((float)(cyInch*cyDIB));
      }
    else
      {
      rcDest.right = cxPage;
      rcDest.top = ((float)(cxPage*cyInch*cyDIB)) /
        ((float)(cxInch*cxDIB));
      }

    CRect rcDestLP(rcDest);
    pDC->DPtoLP(rcDestLP);
    int DPI = this->RenderWindow->GetDPI();

    this->RenderWindow->SetupMemoryRendering(rcDest.right/scale,
                                             rcDest.top/scale,
                                             pDC->m_hAttribDC);

    this->RenderWindow->Render();

    pDC->SetStretchBltMode(HALFTONE);

    StretchBlt(pDC->GetSafeHdc(),0,0,
               rcDest.right, rcDest.top,
               this->RenderWindow->GetMemoryDC(),
               0, 0, rcDest.right/scale, rcDest.top/scale, SRCCOPY);

    this->RenderWindow->ResumeScreenRendering();

    EndWaitCursor();
    }
  else
    {
    this->RenderWindow->Render();
    }
  CView::OnDraw(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// vtkMFCRenderView diagnostics

#ifdef _DEBUG
void vtkMFCRenderView::AssertValid() const
{
  CView::AssertValid();
}

void vtkMFCRenderView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// vtkMFCRenderView message handlers


int vtkMFCRenderView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (vtkMFCView::OnCreate(lpCreateStruct) == -1)
    return -1;

  // TODO: Add your specialized creation code here
  this->RenderWindow->SetParentId(lpCreateStruct->hwndParent);

  return 0;
}

void vtkMFCRenderView::OnInitialUpdate()
{
  vtkMFCView::OnInitialUpdate();

  // TODO: Add your specialized creation code here

  this->RenderWindow->SetWindowId(this->m_hWnd);
  this->RenderWindow->WindowInitialize();
}

// Define our own event handler here
LRESULT vtkMFCRenderView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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
    case WM_MOUSEWHEEL:
    case WM_CHAR:
    case WM_TIMER:
      if (this->Interactor->GetInitialized())
        {
        return vtkHandleMessage2(this->m_hWnd, message, wParam, lParam,
                                 this->Interactor);
        }
      break;
    }
  return vtkMFCView::WindowProc(message, wParam, lParam);
}

void vtkMFCRenderView::OnSize(UINT nType, int cx, int cy)
{
  vtkMFCView::OnSize(nType, cx, cy);

  // TODO: Add your message handler code here
  if (this->Interactor->GetInitialized())
    {
    this->Interactor->SetSize(cx,cy);
    }
}

BOOL vtkMFCRenderView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // TODO: call DoPreparePrinting to invoke the Print dialog box
  // default preparation
  pInfo->SetMinPage(1);
  pInfo->SetMaxPage(1);
  return DoPreparePrinting(pInfo);
}
