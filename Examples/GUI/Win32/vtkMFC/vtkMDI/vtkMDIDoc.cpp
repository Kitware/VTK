/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDIDoc.cpp
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "stdafx.h"
#include "vtkMDI.h"

#include "vtkMDIDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIDoc

IMPLEMENT_DYNCREATE(CVtkMDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CVtkMDIDoc, CDocument)
  //{{AFX_MSG_MAP(CVtkMDIDoc)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIDoc construction/destruction

CVtkMDIDoc::CVtkMDIDoc(): HasAFile(false)
{
  this->Reader = vtkDataSetReader::New();
}

CVtkMDIDoc::~CVtkMDIDoc()
{
  this->Reader->Delete();
}

BOOL CVtkMDIDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // TODO: add reinitialization code here
  // (SDI documents will reuse this document)

  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CVtkMDIDoc serialization

void CVtkMDIDoc::Serialize(CArchive& ar)
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
// CVtkMDIDoc diagnostics

#ifdef _DEBUG
void CVtkMDIDoc::AssertValid() const
{
  CDocument::AssertValid();
}

void CVtkMDIDoc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIDoc commands

BOOL CVtkMDIDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  if (!CDocument::OnOpenDocument(lpszPathName))
    return FALSE;
  
  this->Reader->SetFileName(strdup(lpszPathName));
  this->HasAFile = true;
  
  return TRUE;
}
