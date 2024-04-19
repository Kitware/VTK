// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHardwareWindow
 * @brief   create a window for renderers to draw into
 *
 * vtkHardwareWindow is an abstract object representing a UI based
 * window that can be drawn to. This class is defines an interface that
 * GUI specific subclasses (Win32, X, Cocoa) should support.
 *
 * This class is meant to be Graphics library agnostic. In that it should
 * contain as little graphics library specific code as possible, ideally none.
 * In contrast to classes such as vtkWinOpenGLRenderWindow which contain
 * significant ties to OpenGL.
 *
 */

#ifndef vtkHardwareWindow_h
#define vtkHardwareWindow_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWindow.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkHardwareWindow : public vtkWindow
{
public:
  static vtkHardwareWindow* New();
  vtkTypeMacro(vtkHardwareWindow, vtkWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // create the window (not the instance)
  virtual void Create() {}

  // destroy the window (not the instance)
  virtual void Destroy() {}

protected:
  vtkHardwareWindow();
  ~vtkHardwareWindow() override;

  bool Borders;

private:
  vtkHardwareWindow(const vtkHardwareWindow&) = delete;
  void operator=(const vtkHardwareWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
