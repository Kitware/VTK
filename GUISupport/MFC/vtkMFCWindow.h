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

#ifndef C_VTK_MFC_WINDOW
#define C_VTK_MFC_WINDOW

#include "afxwin.h"

class vtkWin32OpenGLRenderWindow;
class vtkRenderWindowInteractor;

#include "vtkConfigure.h"

#if defined(VTK_BUILD_SHARED_LIBS)
#  if defined(vtkMFC_EXPORTS)
#    define VTK_MFC_EXPORT __declspec( dllexport )
#  else
#    define VTK_MFC_EXPORT __declspec( dllimport )
#  endif
#else
#  define VTK_MFC_EXPORT
#endif

//! class to display a VTK window in an MFC window
class VTK_MFC_EXPORT vtkMFCWindow : public CWnd
{
public:
  //! constructor requires a parent
  vtkMFCWindow(CWnd *pcWnd);
  //! destructor
  virtual ~vtkMFCWindow();

#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  //! draw to a device context
  void DrawDC(CDC* pDC);

  //! give an instance of a vtk render window to the mfc window
  virtual void SetRenderWindow(vtkWin32OpenGLRenderWindow*);
  //! get the render window
  virtual vtkWin32OpenGLRenderWindow* GetRenderWindow();
  //! get the interactor
  virtual vtkRenderWindowInteractor* GetInteractor();

protected:

  //! windows procedure for this class
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

  //! handle size events
  afx_msg void OnSize(UINT nType, int cx, int cy);
  //! handle paint events
  afx_msg void OnPaint();
  //! handle destroy events
  afx_msg void OnDestroy();

  //! don't clear background
  BOOL OnEraseBkgnd(CDC* pDC);

  //! the vtk window
  vtkWin32OpenGLRenderWindow* pvtkWin32OpenGLRW;

  DECLARE_MESSAGE_MAP()
};

#endif
