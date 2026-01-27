// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebAssemblyHardwareWindow
 * @brief   WebAssembly hardware window that interfaces with HTML5 canvas.
 *
 * vtkWebAssemblyHardwareWindow is a concrete implementation of the abstract
 * class vtkHardwareWindow. The class interfaces with an HTML5 canvas element
 * to provide a drawing area for VTK renderers. It uses Emscripten APIs to
 * manage the canvas.
 *
 * @sa vtkHardwareWindow vtkWebGPURenderWindow
 */

#ifndef vtkWebAssemblyHardwareWindow_h
#define vtkWebAssemblyHardwareWindow_h

#if !defined(__EMSCRIPTEN__)
#error "vtkWebAssemblyHardwareWindow cannot be built without emscripten!"
#endif

#include "vtkHardwareWindow.h"

#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;

class VTKRENDERINGUI_EXPORT VTK_MARSHALAUTO vtkWebAssemblyHardwareWindow : public vtkHardwareWindow
{
public:
  static vtkWebAssemblyHardwareWindow* New();
  vtkTypeMacro(vtkWebAssemblyHardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  VTK_NEWINSTANCE
  static vtkOverrideAttribute* CreateOverrideAttributes();

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(vtkTypeBool) override;

  ///@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

  // /**
  //  * Get the current size of the screen in pixels.
  //  */
  // int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  ///@{
  /**
   * These are window system independent methods that are used
   * to help interface vtkWindow to native windowing systems.
   */
  void* GetGenericWindowId() override;
  void* GetGenericDrawable() override;
  ///@}

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() override;
  void ShowCursor() override;
  ///@}

  /**
   * Specify the selector of the canvas element in the DOM.
   */
  vtkGetStringMacro(CanvasSelector);
  vtkSetStringMacro(CanvasSelector);

  /**
   * Make the setter for ShowWindow no-op.
   * This property is meaningless in a web browser context.
   */
  void SetShowWindow(bool) override {}

  void Create() override;
  void Destroy() override;

protected:
  vtkWebAssemblyHardwareWindow();
  ~vtkWebAssemblyHardwareWindow() override;

  void* WindowId = nullptr;
  char* CanvasSelector = nullptr;

private:
  vtkWebAssemblyHardwareWindow(const vtkWebAssemblyHardwareWindow&) = delete;
  void operator=(const vtkWebAssemblyHardwareWindow&) = delete;
};

#define vtkWebAssemblyHardwareWindow_OVERRIDE_ATTRIBUTES                                           \
  vtkWebAssemblyHardwareWindow::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
