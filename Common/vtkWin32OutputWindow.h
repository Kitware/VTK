/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
// Methods from vtkObject
  vtkTypeMacro(vtkWin32OutputWindow,vtkOutputWindow);
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
  vtkWin32OutputWindow(const vtkWin32OutputWindow&);
  void operator=(const vtkWin32OutputWindow&);
  
  void PromptText(const char* text);
  static void AddText(const char*);
  static int Initialize();
  static HWND OutputWindow;
};



#endif
