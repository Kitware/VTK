// vtkMFCView.cpp : implementation file
//

#include "stdafx.h"
#include "vtkMFCView.h"
#include "vtkMFCDocument.h"
#include "resource.h"

#include "vtkWindowToImageFilter.h"
#include "vtkBMPWriter.h"
#include "vtkTIFFWriter.h"
#include "vtkPNMWriter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// vtkMFCView

IMPLEMENT_DYNCREATE(vtkMFCView, CView)

vtkMFCView::vtkMFCView()
{
  this->PrintDPI = 100;
}

vtkMFCView::~vtkMFCView()
{
}


BEGIN_MESSAGE_MAP(vtkMFCView, CView)
	//{{AFX_MSG_MAP(vtkMFCView)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView drawing

void vtkMFCView::OnDraw(CDC* pDC)
{
  CDocument* pDoc = GetDocument();
  // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView diagnostics

#ifdef _DEBUG
void vtkMFCView::AssertValid() const
{
	CView::AssertValid();
}

void vtkMFCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView message handlers

BOOL vtkMFCView::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Add your specialized code here and/or call the base class
  //  the CREATESTRUCT cs
  cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CS_OWNDC;
  return CView::PreCreateWindow(cs);
}

BOOL vtkMFCView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // TODO: call DoPreparePrinting to invoke the Print dialog box
  // default preparation
  pInfo->SetMinPage(1);
  pInfo->SetMaxPage(1);
  return DoPreparePrinting(pInfo);
}

void vtkMFCView::OnEditCopy()
{
  // TODO: Add your command handler code here
  LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
  DWORD               dwLen;      // size of memory block
  HANDLE              hDIB = NULL;  // handle to DIB, temp handle
  vtkWindow *vtkWin = this->GetVTKWindow();
  int *size = vtkWin->GetSize();
  int dataWidth = ((size[0]*3+3)/4)*4;

  if (OpenClipboard())
    {
    BeginWaitCursor();
    EmptyClipboard();

    dwLen = sizeof(BITMAPINFOHEADER) + dataWidth*size[1];
    hDIB = ::GlobalAlloc(GHND, dwLen);
    lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(hDIB);

    lpbi->biSize = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth = size[0];
    lpbi->biHeight = size[1];
    lpbi->biPlanes = 1;
    lpbi->biBitCount = 24;
    lpbi->biCompression = BI_RGB;
    lpbi->biClrUsed = 0;
    lpbi->biClrImportant = 0;
    lpbi->biSizeImage = dataWidth*size[1];

    this->SetupMemoryRendering(size[0],size[1],
                               this->GetDC()->GetSafeHdc());
    vtkWin->Render();

    memcpy((LPSTR)lpbi + lpbi->biSize,
           this->GetMemoryData(),dataWidth*size[1]);

    SetClipboardData (CF_DIB, hDIB);
    ::GlobalUnlock(hDIB);
    CloseClipboard();
    this->ResumeScreenRendering();
    EndWaitCursor();
    }
}

