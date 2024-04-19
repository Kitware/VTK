// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStringOutputWindow
 * @brief   File Specific output window class
 *
 * Writes debug/warning/error output to a log file instead of the console.
 * To use this class, instantiate it and then call SetInstance(this).
 *
 */

#ifndef vtkStringOutputWindow_h
#define vtkStringOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"
#include <sstream> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkStringOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkStringOutputWindow, vtkOutputWindow);

  static vtkStringOutputWindow* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Put the text into the log file.
   * New lines are converted to carriage return new lines.
   */
  void DisplayText(const char*) override;

  /**
   * Get the current output as a string
   */
  std::string GetOutput() { return this->OStream.str(); }

protected:
  vtkStringOutputWindow();
  ~vtkStringOutputWindow() override;
  void Initialize();

  std::ostringstream OStream;

private:
  vtkStringOutputWindow(const vtkStringOutputWindow&) = delete;
  void operator=(const vtkStringOutputWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
