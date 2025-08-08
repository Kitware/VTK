// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCocoaHardwareWindow
 * @brief   represents a window in a Cocoa GUI
 *
 * vtkCocoaHardwareWindow is a class for managing a native macOS window.
 * It is backed by an NSWindow and its view is configured with a CAMetalLayer,
 * making it suitable for Metal-based rendering.
 */

#ifndef vtkCocoaHardwareWindow_h
#define vtkCocoaHardwareWindow_h

#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro

// Forward declare Objective-C classes
#ifdef __OBJC__
@class NSWindow;
@class NSView;
@class vtkCocoaWindowDelegate;
#else
class NSWindow;
class NSView;
class vtkCocoaWindowDelegate;
#endif

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGUI_EXPORT vtkCocoaHardwareWindow : public vtkHardwareWindow
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCocoaHardwareWindow* New();
  vtkTypeMacro(vtkCocoaHardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the native Cocoa window object.
   */
  NSWindow* GetWindowId();

  /**
   * Get the native Cocoa view object.
   */
  NSView* GetViewId();

  /**
   * Get the metal layer object.
   */
  void* GetMetalLayer();

  // vtkHardwareWindow overrides
  void Create() override;
  void Destroy() override;

  void* GetGenericWindowId() override;
  void* GetGenericParentId() override;

  ///@{
  /**
   * Set the size of the window in screen coordinates.
   */
  void SetSize(int, int) override;
  using vtkHardwareWindow::SetSize;
  ///@}

  ///@{
  /**
   * Set the position of the window in screen coordinates.
   */
  void SetPosition(int, int) override;
  using vtkHardwareWindow::SetPosition;
  ///@}

  /**
   * Set the name (title) of the window.
   */
  void SetWindowName(const char*) override;

  ///@{
  /**
   * Hide or show the mouse cursor.
   */
  void HideCursor() override;
  void ShowCursor() override;
  ///@}

  /**
   * Change the shape of the cursor.
   */
  void SetCurrentCursor(int) override;

protected:
  vtkCocoaHardwareWindow();
  ~vtkCocoaHardwareWindow() override;

  NSWindow* WindowId = nullptr;
  NSView* ViewId = nullptr;
  vtkCocoaWindowDelegate* Delegate = nullptr;

  bool OwnsWindow = false;
  bool CursorHidden = false;

private:
  vtkCocoaHardwareWindow(const vtkCocoaHardwareWindow&) = delete;
  void operator=(const vtkCocoaHardwareWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCocoaHardwareWindow_h
