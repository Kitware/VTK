/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ProcessOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWin32ProcessOutputWindow - Win32-specific output window class
// .SECTION Description
// vtkWin32ProcessOutputWindow executes a process and sends messages
// to its standard input pipe.  This is useful to have a separate
// process display VTK errors so that if a VTK application crashes,
// the error messages are still available.

#ifndef vtkWin32ProcessOutputWindow_h
#define vtkWin32ProcessOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"

class VTKCOMMONCORE_EXPORT vtkWin32ProcessOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkWin32ProcessOutputWindow,vtkOutputWindow);
  static vtkWin32ProcessOutputWindow* New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Send text to the output window process.
  virtual void DisplayText(const char*);

protected:
  vtkWin32ProcessOutputWindow();
  ~vtkWin32ProcessOutputWindow();

  int Initialize();
  void Write(const char* data, size_t length);

  // The write end of the pipe to the child process.
  vtkWindowsHANDLE OutputPipe;

  // Whether the pipe has been broken.
  int Broken;

  // Count the number of times a new child has been initialized.
  unsigned int Count;
private:
  vtkWin32ProcessOutputWindow(const vtkWin32ProcessOutputWindow&);  // Not implemented.
  void operator=(const vtkWin32ProcessOutputWindow&);  // Not implemented.
};

#endif
