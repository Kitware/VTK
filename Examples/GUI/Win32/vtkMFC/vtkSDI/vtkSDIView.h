/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSDIView.h
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

#if !defined(AFX_VTKSDIVIEW_H__73D7D5BB_8FEB_4452_A5EB_964ECEEE2A83__INCLUDED_)
#define AFX_VTKSDIVIEW_H__73D7D5BB_8FEB_4452_A5EB_964ECEEE2A83__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Include the required header files for the vtk classes we are using
#include <vtkRenderer.h>
#include <vtkWin32OpenGLRenderWindow.h>
#include <vtkWin32RenderWindowInteractor.h>

#include <vtkSphereSource.h>
#include <vtkConeSource.h>
#include <vtkGlyph3D.h>
#include <vtkElevationFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkCubeAxesActor2D.h>

class CVtkSDIView : public CView
{
protected: // create from serialization only
  CVtkSDIView();
  DECLARE_DYNCREATE(CVtkSDIView)

// Attributes
public:
  CVtkSDIDoc* GetDocument();

// Operations
public:

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CVtkSDIView)
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
  void Pipeline ( void );
  virtual ~CVtkSDIView();

  vtkWin32OpenGLRenderWindow *renWin;
  vtkRenderer *ren;
  vtkWin32RenderWindowInteractor *iren;
  
  vtkSphereSource *sphere;
  vtkPolyDataMapper *sphereMapper;
  vtkElevationFilter *sphereElevation;
  vtkActor *sphereActor;
  vtkConeSource *cone;
  vtkGlyph3D *glyph;
  vtkPolyDataMapper *spikeMapper;
  vtkActor *spikeActor;
  vtkCubeAxesActor2D *sphereAxis;


#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
  //{{AFX_MSG(CVtkSDIView)
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vtkSDIView.cpp
inline CVtkSDIDoc* CVtkSDIView::GetDocument()
   { return (CVtkSDIDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKSDIVIEW_H__73D7D5BB_8FEB_4452_A5EB_964ECEEE2A83__INCLUDED_)
