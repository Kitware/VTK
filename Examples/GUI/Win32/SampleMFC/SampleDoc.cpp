// SampleDoc.cpp : implementation of the CSampleDoc class
//

#include "stdafx.h"
#include "Sample.h"

#include "SampleDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "vtkTextMapper.h"
#include "vtkActor2D.h"
#include "vtkTextProperty.h"
/////////////////////////////////////////////////////////////////////////////
// CSampleDoc

IMPLEMENT_DYNCREATE(CSampleDoc, vtkMFCDocument)

BEGIN_MESSAGE_MAP(CSampleDoc, vtkMFCDocument)
	//{{AFX_MSG_MAP(CSampleDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSampleDoc construction/destruction

CSampleDoc::CSampleDoc()
{
	// TODO: add one-time construction code here
  this->Reader = vtkDataSetReader::New();
  this->Mapper = vtkDataSetMapper::New();
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->VisibilityOff();
  this->Props->AddItem(this->Actor);

  vtkActor2D *act = vtkActor2D::New();
  vtkTextMapper *txt = vtkTextMapper::New();
  act->SetMapper(txt);
  txt->SetInput("Hello World");
  txt->GetTextProperty()->SetFontSize(24);
  this->Props->AddItem(act);
  txt->Delete();
  act->Delete();
}

CSampleDoc::~CSampleDoc()
{
  this->Reader->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
}

BOOL CSampleDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CSampleDoc serialization

void CSampleDoc::Serialize(CArchive& ar)
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
// CSampleDoc diagnostics

#ifdef _DEBUG
void CSampleDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSampleDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSampleDoc commands

BOOL CSampleDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  if (!vtkMFCDocument::OnOpenDocument(lpszPathName))
    {
    return FALSE;
    }
  
  // TODO: Add your specialized creation code here
  this->Reader->SetFileName(lpszPathName);
  this->Mapper->SetInput(this->Reader->GetOutput());
  this->Actor->VisibilityOn();
	
  return TRUE;
}
