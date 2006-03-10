// vtkSDIView.h : interface of the CvtkSDIView class
//


#pragma once

#include "vtkMFCWindow.h"

class CvtkSDIView : public CView
{
public:
  virtual ~CvtkSDIView();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  CvtkSDIDoc* GetDocument() const;
  vtkRenderer* GetRenderer() { ASSERT(pvtkRenderer); return pvtkRenderer; }

  virtual void OnDraw(CDC* pDC);  // overridden to draw this view

  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  afx_msg BOOL OnEraseBkgnd(CDC* pDC);
  afx_msg void OnSize(UINT nType, int cx, int cy);

private:
  vtkRenderer            *pvtkRenderer;
  vtkMFCWindow          *pvtkMFCWindow;

protected:
  DECLARE_DYNCREATE(CvtkSDIView)
  CvtkSDIView();

  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

  DECLARE_MESSAGE_MAP()
public:
  virtual void OnInitialUpdate();
};

#ifndef _DEBUG  // debug version in vtkSDIView.cpp
inline CvtkSDIDoc* CvtkSDIView::GetDocument() const
   { return reinterpret_cast<CvtkSDIDoc*>(m_pDocument); }
#endif

