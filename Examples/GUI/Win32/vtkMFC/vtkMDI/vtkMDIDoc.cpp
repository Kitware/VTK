// vtkMDIDoc.cpp : implementation of the CvtkMDIDoc class
//

#include "stdafx.h"
#include "vtkMDI.h"

#include "vtkMDIDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CvtkMDIDoc

IMPLEMENT_DYNCREATE(CvtkMDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CvtkMDIDoc, CDocument)
END_MESSAGE_MAP()


// CvtkMDIDoc construction/destruction

CvtkMDIDoc::CvtkMDIDoc()
{
  // TODO: add one-time construction code here
  this->pvtkDataSetReader = NULL;
}

CvtkMDIDoc::~CvtkMDIDoc()
{
}

BOOL CvtkMDIDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // TODO: add reinitialization code here
  // (SDI documents will reuse this document)

  return TRUE;
}




// CvtkMDIDoc serialization

void CvtkMDIDoc::Serialize(CArchive& ar)
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


// CvtkMDIDoc diagnostics

#ifdef _DEBUG
void CvtkMDIDoc::AssertValid() const
{
  CDocument::AssertValid();
}

void CvtkMDIDoc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG


// CvtkMDIDoc commands

BOOL CvtkMDIDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
  if (!CDocument::OnOpenDocument(lpszPathName))
    return FALSE;

  this->pvtkDataSetReader = vtkDataSetReader::New();
  this->pvtkDataSetReader->SetFileName(lpszPathName);

  return TRUE;
}

void CvtkMDIDoc::OnCloseDocument()
{
  // delete data
  if (this->pvtkDataSetReader)  this->pvtkDataSetReader->Delete();

  CDocument::OnCloseDocument();
}
