/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.h
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
// .NAME vtkWin32OutputWindow - Win32 Specific output window class
// .SECTION Description
// This class is used for error and debug message output on the windows
// platform.   It creates a read only EDIT control to display the
// output.   This class should not be used directly.   It should
// only be used through the interface of vtkOutputWindow.  This class
// only handles one output window per process.  If the window is destroyed,
// the vtkObject::GlobalWarningDisplayOff() function is called.  The
// window is created the next time text is written to the window.

#ifndef __vtkWin32OutputWindow_h
#define __vtkWin32OutputWindow_h

#include "vtkOutputWindow.h"


class VTK_COMMON_EXPORT vtkWin32OutputWindow : public vtkOutputWindow
{
public:
// Methods from vtkObject
  vtkTypeRevisionMacro(vtkWin32OutputWindow,vtkOutputWindow);
  // Description:
  // Create a vtkWin32OutputWindow.
  static vtkWin32OutputWindow* New();
  // Description:  Put the text into the display window.
  // New lines are converted to carriage return new lines.
  virtual void DisplayText(const char*);
  //BTX
  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, 
                                  WPARAM wParam, LPARAM lParam);
  //ETX
protected: 
  vtkWin32OutputWindow() {}; 
  virtual ~vtkWin32OutputWindow() {}; 
  
  void PromptText(const char* text);
  static void AddText(const char*);
  static int Initialize();
  static HWND OutputWindow;
private:
  vtkWin32OutputWindow(const vtkWin32OutputWindow&);  // Not implemented.
  void operator=(const vtkWin32OutputWindow&);  // Not implemented.
};



#endif
