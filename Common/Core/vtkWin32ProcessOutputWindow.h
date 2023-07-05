// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32ProcessOutputWindow
 * @brief   Win32-specific output window class
 *
 * vtkWin32ProcessOutputWindow executes a process and sends messages
 * to its standard input pipe.  This is useful to have a separate
 * process display VTK errors so that if a VTK application crashes,
 * the error messages are still available.
 */

#ifndef vtkWin32ProcessOutputWindow_h
#define vtkWin32ProcessOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkWin32ProcessOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkWin32ProcessOutputWindow, vtkOutputWindow);
  static vtkWin32ProcessOutputWindow* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Send text to the output window process.
   */
  void DisplayText(const char*) override;

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
  vtkWin32ProcessOutputWindow(const vtkWin32ProcessOutputWindow&) = delete;
  void operator=(const vtkWin32ProcessOutputWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
