/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDIDoc.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if !defined(AFX_VTKMDIDOC_H__0865564D_A766_43E2_849B_7356BAC7A374__INCLUDED_)
#define AFX_VTKMDIDOC_H__0865564D_A766_43E2_849B_7356BAC7A374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Include the required header files for the vtk classes we are using
#include <vtkDataSetReader.h>

class CVtkMDIDoc : public CDocument
{
protected: // create from serialization only
  CVtkMDIDoc();
  DECLARE_DYNCREATE(CVtkMDIDoc)

// Attributes
public:

// Operations
public:

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CVtkMDIDoc)
  public:
  virtual BOOL OnNewDocument();
  virtual void Serialize(CArchive& ar);
  virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
  //}}AFX_VIRTUAL

// Implementation
public:
  virtual ~CVtkMDIDoc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

public:
  bool HasAFile;
  vtkDataSetReader *Reader;


// Generated message map functions
protected:
  //{{AFX_MSG(CVtkMDIDoc)
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code !
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMDIDOC_H__0865564D_A766_43E2_849B_7356BAC7A374__INCLUDED_)
