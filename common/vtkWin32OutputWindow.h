/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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


class VTK_EXPORT vtkWin32OutputWindow : public vtkOutputWindow
{
public:
  static vtkWin32OutputWindow* New();
  // Description:  Put the text into the display window.
  // New lines are converted to carriage return new lines.
  virtual void DisplayText(const char*);
  //BTX
  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, 
				  WPARAM wParam, LPARAM lParam);
  //ETX
private: 
  vtkWin32OutputWindow() {}; 
  virtual ~vtkWin32OutputWindow() {}; 
  vtkWin32OutputWindow(const vtkWin32OutputWindow&) {};
  void operator=(const vtkWin32OutputWindow&) {};
  
  void PromptText(const char* text);
  static void AddText(const char*);
  static int Initialize();
  static HWND OutputWindow;
};



#endif
