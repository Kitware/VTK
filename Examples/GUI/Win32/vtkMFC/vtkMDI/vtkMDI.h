/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if !defined(AFX_VTKMDI_H__FBE4E818_EBEB_45D3_8269_3C4D8E2E5179__INCLUDED_)
#define AFX_VTKMDI_H__FBE4E818_EBEB_45D3_8269_3C4D8E2E5179__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
  #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CVtkMDIApp:
// See vtkMDI.cpp for the implementation of this class
//

class CVtkMDIApp : public CWinApp
{
public:
  CVtkMDIApp();

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CVtkMDIApp)
  public:
  virtual BOOL InitInstance();
  //}}AFX_VIRTUAL

// Implementation
  //{{AFX_MSG(CVtkMDIApp)
  afx_msg void OnAppAbout();
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code !
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMDI_H__FBE4E818_EBEB_45D3_8269_3C4D8E2E5179__INCLUDED_)
