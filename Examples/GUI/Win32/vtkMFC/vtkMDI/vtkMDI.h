// vtkMDI.h : main header file for the vtkMDI application
//
#pragma once

#ifndef __AFXWIN_H__
  #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CvtkMDIApp:
// See vtkMDI.cpp for the implementation of this class
//

class CvtkMDIApp : public CWinApp
{
public:
  CvtkMDIApp();


// Overrides
public:
  virtual BOOL InitInstance();

// Implementation
  afx_msg void OnAppAbout();
  DECLARE_MESSAGE_MAP()
};

extern CvtkMDIApp theApp;
