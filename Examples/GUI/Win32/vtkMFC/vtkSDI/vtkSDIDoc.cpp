/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSDIDoc.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "stdafx.h"
#include "vtkSDI.h"

#include "vtkSDIDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIDoc

IMPLEMENT_DYNCREATE(CVtkSDIDoc, CDocument)

BEGIN_MESSAGE_MAP(CVtkSDIDoc, CDocument)
  //{{AFX_MSG_MAP(CVtkSDIDoc)
    // NOTE - the ClassWizard will add and remove mapping macros here.
    //    DO NOT EDIT what you see in these blocks of generated code!
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIDoc construction/destruction

CVtkSDIDoc::CVtkSDIDoc()
{
  // TODO: add one-time construction code here

}

CVtkSDIDoc::~CVtkSDIDoc()
{
}

BOOL CVtkSDIDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // TODO: add reinitialization code here
  // (SDI documents will reuse this document)

  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CVtkSDIDoc serialization

void CVtkSDIDoc::Serialize(CArchive& ar)
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
// CVtkSDIDoc diagnostics

#ifdef _DEBUG
void CVtkSDIDoc::AssertValid() const
{
  CDocument::AssertValid();
}

void CVtkSDIDoc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVtkSDIDoc commands
