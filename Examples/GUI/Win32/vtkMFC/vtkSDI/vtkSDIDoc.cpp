// vtkSDIDoc.cpp : implementation of the CvtkSDIDoc class
//

#include "stdafx.h"
#include "vtkSDI.h"

#include "vtkSDIDoc.h"
#include "vtkSDIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CvtkSDIDoc

IMPLEMENT_DYNCREATE(CvtkSDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CvtkSDIDoc, CDocument)
END_MESSAGE_MAP()


// CvtkSDIDoc construction/destruction

CvtkSDIDoc::CvtkSDIDoc()
{
  this->pvtkDataSetReader  = NULL;

  // Create the the objects used to form the visualisation.
  this->pvtkDataSetMapper  = vtkDataSetMapper::New();
  this->pvtkActor      = vtkActor::New();
  this->pvtkActor2D    = vtkActor2D::New();
  this->pvtkTextMapper  = vtkTextMapper::New();
}

CvtkSDIDoc::~CvtkSDIDoc()
{
}

BOOL CvtkSDIDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // remove old actors
  RemoveActors();

  // execute object pipeline
  ExecutePipeline();

  return TRUE;
}




// CvtkSDIDoc serialization

void CvtkSDIDoc::Serialize(CArchive& ar)
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


// CvtkSDIDoc diagnostics

#ifdef _DEBUG
void CvtkSDIDoc::AssertValid() const
{
  CDocument::AssertValid();
}

void CvtkSDIDoc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG


// CvtkSDIDoc commands

void CvtkSDIDoc::RemoveActors()
{
  // get our renderer first
  POSITION pos = this->GetFirstViewPosition();
  CvtkSDIView *pcvtkSDIView = NULL;

  if (pos)
  {
    pcvtkSDIView = (CvtkSDIView *)GetNextView(pos);
  }
  else  // return
  {
    ASSERT(FALSE);
    return;
  }

  // remove old actors
  pcvtkSDIView->GetRenderer()->RemoveActor(this->pvtkActor);
  pcvtkSDIView->GetRenderer()->RemoveActor(this->pvtkActor2D);
}

BOOL CvtkSDIDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
  if (!CDocument::OnOpenDocument(lpszPathName))
    return FALSE;

  // remove old actors
  RemoveActors();

  // create new data reader
  this->pvtkDataSetReader = vtkDataSetReader::New();
  this->pvtkDataSetReader->SetFileName(lpszPathName);

  // execute object pipeline
  ExecutePipeline();

  return TRUE;
}

void CvtkSDIDoc::OnCloseDocument()
{
  // delete data
  if (this->pvtkDataSetReader)  this->pvtkDataSetReader->Delete();

  // Delete the the objects used to form the visualisation.
  if (this->pvtkDataSetMapper)  this->pvtkDataSetMapper->Delete();
  if (this->pvtkActor)      this->pvtkActor->Delete();
  if (this->pvtkActor2D)      this->pvtkActor2D->Delete();
  if (this->pvtkTextMapper)    this->pvtkTextMapper->Delete();

  CDocument::OnCloseDocument();
}

void CvtkSDIDoc::ExecutePipeline()
{
  // get our renderer first
  POSITION pos = this->GetFirstViewPosition();
  CvtkSDIView *pcvtkSDIView = NULL;

  if (pos)
  {
    pcvtkSDIView = (CvtkSDIView *)GetNextView(pos);
  }
  else  // return
  {
    ASSERT(FALSE);
    return;
  }

  if (pvtkDataSetReader)  // have file
  {
    this->pvtkDataSetMapper->SetInput(this->pvtkDataSetReader->GetOutput());
    this->pvtkActor->SetMapper(this->pvtkDataSetMapper);

    this->pvtkTextMapper->SetInput(this->pvtkDataSetReader->GetFileName());
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(12);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    pcvtkSDIView->GetRenderer()->SetBackground(0.0,0.0,0.4);
    pcvtkSDIView->GetRenderer()->AddActor(this->pvtkActor);
    pcvtkSDIView->GetRenderer()->AddActor(this->pvtkActor2D);
    pcvtkSDIView->GetRenderer()->ResetCamera();
    pvtkDataSetReader->Delete();
    pvtkDataSetReader = NULL;
  }
  else  // have no file
  {
    this->pvtkTextMapper->SetInput("Hello World");
    this->pvtkTextMapper->GetTextProperty()->SetFontSize(24);
    this->pvtkActor2D->SetMapper(this->pvtkTextMapper);

    pcvtkSDIView->GetRenderer()->SetBackground(0.0,0.0,0.4);
    pcvtkSDIView->GetRenderer()->AddActor(this->pvtkActor2D);
    pcvtkSDIView->GetRenderer()->ResetCamera();
  }
}
