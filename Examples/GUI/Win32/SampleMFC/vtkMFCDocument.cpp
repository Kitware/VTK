// vtkMFCDocument.cpp : implementation file
//

#include "stdafx.h"
#include "Sample.h"
#include "vtkMFCDocument.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// vtkMFCDocument

IMPLEMENT_DYNCREATE(vtkMFCDocument, CDocument)

vtkMFCDocument::vtkMFCDocument()
{
  this->Props = vtkPropCollection::New();
}

BOOL vtkMFCDocument::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

vtkMFCDocument::~vtkMFCDocument()
{
  if (this->Props) this->Props->Delete();
}


BEGIN_MESSAGE_MAP(vtkMFCDocument, CDocument)
	//{{AFX_MSG_MAP(vtkMFCDocument)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// vtkMFCDocument diagnostics

#ifdef _DEBUG
void vtkMFCDocument::AssertValid() const
{
	CDocument::AssertValid();
}

void vtkMFCDocument::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// vtkMFCDocument serialization

void vtkMFCDocument::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// vtkMFCDocument commands

