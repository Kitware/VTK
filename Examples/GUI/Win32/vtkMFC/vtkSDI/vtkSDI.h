// vtkSDI.h : main header file for the vtkSDI application
//
#pragma once

#ifndef __AFXWIN_H__
  #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CvtkSDIApp:
// See vtkSDI.cpp for the implementation of this class
//

class CvtkSDIApp : public CWinApp
{
public:
  CvtkSDIApp();


// Overrides
public:
  virtual BOOL InitInstance();

// Implementation
  afx_msg void OnAppAbout();
  DECLARE_MESSAGE_MAP()
};

extern CvtkSDIApp theApp;
