/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFCRenderView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if !defined(AFX_VTKMFCRENDERVIEW_H__AEB91BA7_8091_11D2_985E_00A0CC243C06__INCLUDED_)
#define AFX_VTKMFCRENDERVIEW_H__AEB91BA7_8091_11D2_985E_00A0CC243C06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// vtkMFCRenderView.h : header file
//
#include "vtkMFCView.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkWin32RenderWindowInteractor.h"

/////////////////////////////////////////////////////////////////////////////
// vtkMFCRenderView view

class vtkMFCRenderView : public vtkMFCView
{
protected:
  vtkMFCRenderView();           // protected constructor used by dynamic creation
  DECLARE_DYNCREATE(vtkMFCRenderView)

    vtkRenderer *Renderer;
  vtkWin32OpenGLRenderWindow *RenderWindow;
  vtkWin32RenderWindowInteractor *Interactor;

  // Attributes
public:

  // Operations
public:
  virtual void OnInitialUpdate();
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  void Render() {this->RenderWindow->Render();};
  vtkWindow *GetVTKWindow() {return this->RenderWindow;};
  virtual void SetupMemoryRendering(int x, int y, HDC prn) {
    this->RenderWindow->SetupMemoryRendering(x,y,prn);};
  virtual void ResumeScreenRendering() {
    this->RenderWindow->ResumeScreenRendering();};
  virtual unsigned char *GetMemoryData() {
    return this->RenderWindow->GetMemoryData();};

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(vtkMFCRenderView)
protected:
  virtual void OnDraw(CDC* pDC);      // overridden to draw this view
  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  //}}AFX_VIRTUAL

  // Implementation
protected:
  virtual ~vtkMFCRenderView();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  // Generated message map functions
protected:
  //{{AFX_MSG(vtkMFCRenderView)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
    };

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VTKMFCRENDERVIEW_H__AEB91BA7_8091_11D2_985E_00A0CC243C06__INCLUDED_)
