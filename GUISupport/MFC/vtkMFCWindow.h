/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if !defined(C_VTK_MFC_WINDOW)
#define C_VTK_MFC_WINDOW

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "vtkMFCStdAfx.h"

// Include the required header files for the vtk classes we are using
class vtkWin32OpenGLRenderWindow;
class vtkWin32RenderWindowInteractor;

class VTK_MFC_EXPORT vtkMFCWindow : public CWnd
{
public:
  vtkMFCWindow();
  virtual ~vtkMFCWindow();

  DECLARE_DYNCREATE(vtkMFCWindow)

// Operations
public:
  vtkWin32OpenGLRenderWindow* GetRenderWindow();

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(vtkMFCWindow)
  public:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  protected:
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  //}}AFX_VIRTUAL

// Implementation
protected:
  vtkWin32OpenGLRenderWindow *renWin;
  vtkWin32RenderWindowInteractor *iren;
  
public:
  
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
  //{{AFX_MSG(vtkMFCWindow)
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnPaint();  // overridden to draw this view
  BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(C_VTK_MFC_VIEW)
