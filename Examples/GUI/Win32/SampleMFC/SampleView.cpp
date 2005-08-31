// SampleView.cpp : implementation of the CSampleView class
//

#include "stdafx.h"
#include "Sample.h"

#include "SampleDoc.h"
#include "SampleView.h"

#include "vtkRenderer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSampleView

IMPLEMENT_DYNCREATE(CSampleView, vtkMFCRenderView)

BEGIN_MESSAGE_MAP(CSampleView, vtkMFCRenderView)
  //{{AFX_MSG_MAP(CSampleView)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSampleView construction/destruction

CSampleView::CSampleView()
{
  // TODO: add construction code here

}

CSampleView::~CSampleView()
{
}


/////////////////////////////////////////////////////////////////////////////
// CSampleView diagnostics

#ifdef _DEBUG
void CSampleView::AssertValid() const
{
  CView::AssertValid();
}

void CSampleView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CSampleDoc* CSampleView::GetDocument() // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSampleDoc)));
  return (CSampleDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSampleView message handlers

void CSampleView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
  // TODO: Add your specialized code here and/or call the base class
  vtkPropCollection *propc;
  vtkProp *prop;
  // TODO: Add your specialized code here and/or call the base class
  // first remove any old actors
  this->Renderer->GetViewProps()->RemoveAllItems();
  propc = this->GetDocument()->GetViewProps();
  propc->InitTraversal();
  while (prop = propc->GetNextProp())
    {
    this->Renderer->AddViewProp(prop);
    this->Renderer->ResetCamera();
    }
  
  this->vtkMFCRenderView::OnUpdate(pSender, lHint, pHint);      
}
