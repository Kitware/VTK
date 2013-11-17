/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if !defined(AFX_VTKMFCVIEW_H__5D36A981_8081_11D2_985E_00A0CC243C06__INCLUDED_)
#define AFX_VTKMFCVIEW_H__5D36A981_8081_11D2_985E_00A0CC243C06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// vtkMFCView.h : header file
//
#define vtkMFCSetObjectMacro(name,type) \
void Set##name (type* _arg) \
  { \
  if (this->name != _arg) \
    { \
    if (this->name != NULL) { this->name->UnRegister(NULL); }\
    this->name = _arg; \
    if (this->name != NULL) { this->name->Register(NULL); } \
    } \
  }

/////////////////////////////////////////////////////////////////////////////
// vtkMFCView view
class vtkMFCDocument;
#include "vtkWindow.h"

class vtkMFCView : public CView
{
protected:
  vtkMFCView();           // protected constructor used by dynamic creation
  DECLARE_DYNCREATE(vtkMFCView)
    int PrintDPI;

// Attributes
public:

// Operations
public:
  void SetPrintDPI(int dpi) {this->PrintDPI = dpi;};
  int  GetPrintDPI() {return this->PrintDPI;};
  vtkMFCDocument *GetDocument() {return (vtkMFCDocument *)m_pDocument;};
  virtual vtkWindow *GetVTKWindow() {return NULL;};
  virtual void SetupMemoryRendering(int x, int y, HDC prn) {}
  virtual void ResumeScreenRendering() {}
  virtual unsigned char *GetMemoryData() {return NULL;};

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(vtkMFCView)
protected:
  virtual void OnDraw(CDC* pDC);      // overridden to draw this view
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  //}}AFX_VIRTUAL

// Implementation
protected:
  virtual ~vtkMFCView();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  // Generated message map functions
protected:
  //{{AFX_MSG(vtkMFCView)
  afx_msg void OnEditCopy();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
    };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMFCVIEW_H__5D36A981_8081_11D2_985E_00A0CC243C06__INCLUDED_)
