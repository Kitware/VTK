// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32OutputWindow
 * @brief   Win32 Specific output window class
 *
 * This class is used for error and debug message output on the Windows
 * platform.   It creates a read only EDIT control to display the
 * output.   This class should not be used directly.   It should
 * only be used through the interface of vtkOutputWindow.  This class
 * only handles one output window per process.  If the window is destroyed,
 * the vtkObject::GlobalWarningDisplayOff() function is called.  The
 * window is created the next time text is written to the window.
 *
 * In its constructor, vtkWin32OutputWindow changes the default
 * `vtkOutputWindow::DisplayMode` to
 * `vtkOutputWindow::NEVER` unless running on a dashboard machine,
 * in which cause it's left as `vtkOutputWindow::DEFAULT`.
 */

#ifndef vtkWin32OutputWindow_h
#define vtkWin32OutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkWin32OutputWindow : public vtkOutputWindow
{
public:
  // Methods from vtkObject
  vtkTypeMacro(vtkWin32OutputWindow, vtkOutputWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Create a vtkWin32OutputWindow.
   */
  static vtkWin32OutputWindow* New();

  /**
   * New lines are converted to carriage return new lines.
   */
  void DisplayText(const char*) override;

  /**
   * Returns the window title.
   */
  virtual const char* GetWindowTitle() { return "vtkOutputWindow"; }

protected:
  vtkWin32OutputWindow();
  ~vtkWin32OutputWindow() override;

  virtual void PromptText(const char* text);
  virtual void AddText(const char*);
  virtual int Initialize();

private:
  vtkWin32OutputWindow(const vtkWin32OutputWindow&) = delete;
  void operator=(const vtkWin32OutputWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
