/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMDIView.h
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

#if !defined(AFX_VTKMDIVIEW_H__5A346CD5_5968_4619_BB2C_17A2D5F6E367__INCLUDED_)
#define AFX_VTKMDIVIEW_H__5A346CD5_5968_4619_BB2C_17A2D5F6E367__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Include the required header files for the vtk classes we are using
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkTextMapper.h>

#include <vtkRenderer.h>
#include <vtkWin32OpenGLRenderWindow.h>
#include <vtkWin32RenderWindowInteractor.h>

class CVtkMDIView : public CView
{
protected: // create from serialization only
  CVtkMDIView();
  DECLARE_DYNCREATE(CVtkMDIView)

// Attributes
public:
  CVtkMDIDoc* GetDocument();

// Operations
public:

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CVtkMDIView)
  public:
  virtual void OnDraw(CDC* pDC);  // overridden to draw this view
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  protected:
  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  //}}AFX_VIRTUAL

// Implementation
private:
  void Pipeline( void );
  vtkWin32OpenGLRenderWindow *renWin;
  vtkRenderer *ren;
  vtkWin32RenderWindowInteractor *iren;
  
  vtkDataSetMapper *Mapper;
  vtkActor *Actor;
  
  vtkActor2D *txtActor;
  vtkTextMapper *txtMapper;

public:
  virtual ~CVtkMDIView();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
  //{{AFX_MSG(CVtkMDIView)
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vtkMDIView.cpp
inline CVtkMDIDoc* CVtkMDIView::GetDocument()
   { return (CVtkMDIDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMDIVIEW_H__5A346CD5_5968_4619_BB2C_17A2D5F6E367__INCLUDED_)
