// vtkMDIView.h : interface of the CvtkMDIView class
//


#pragma once

#include "vtkMFCWindow.h"

class CvtkMDIView : public CView
{
protected: // create from serialization only
  CvtkMDIView();
  DECLARE_DYNCREATE(CvtkMDIView)

// Attributes
public:
  virtual ~CvtkMDIView();
  CvtkMDIDoc* GetDocument() const;
  virtual void OnDraw(CDC* pDC);  // overridden to draw this view

#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

private:
  void ExecutePipeline();

  vtkMFCWindow          *pvtkMFCWindow;

  vtkRenderer            *pvtkRenderer;
  vtkDataSetMapper        *pvtkDataSetMapper;
  vtkActor            *pvtkActor;
  vtkActor2D            *pvtkActor2D;
  vtkTextMapper          *pvtkTextMapper;

public:
  virtual void OnInitialUpdate();
  afx_msg void OnDestroy();
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnSize(UINT nType, int cx, int cy);

protected:
  BOOL OnEraseBkgnd(CDC* pDC);
  virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
  virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
  virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
  DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in vtkMDIView.cpp
inline CvtkMDIDoc* CvtkMDIView::GetDocument() const
   { return reinterpret_cast<CvtkMDIDoc*>(m_pDocument); }
#endif

