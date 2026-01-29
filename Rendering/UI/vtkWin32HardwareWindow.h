// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32HardwareWindow
 * @brief   represents a window in a windows GUI
 */

#ifndef vtkWin32HardwareWindow_h
#define vtkWin32HardwareWindow_h

#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro
#include "vtkWindows.h"           // For windows API
#include "vtkWrappingHints.h"     // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGUI_EXPORT VTK_MARSHALAUTO vtkWin32HardwareWindow : public vtkHardwareWindow
{
public:
  static vtkWin32HardwareWindow* New();
  vtkTypeMacro(vtkWin32HardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  HINSTANCE GetApplicationInstance();

  HWND GetWindowId();

  void Create() override;
  void Destroy() override;

  ///@{
  /**
   * These are window system independent methods that are used
   * to help interface vtkWindow to native windowing systems.
   */
  void SetDisplayId(void*) override;
  void SetWindowId(void*) override;
  void SetParentId(void*) override;
  void* GetGenericDisplayId() override;
  void* GetGenericWindowId() override;
  void* GetGenericParentId() override;
  ///@}

  ///@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  using vtkHardwareWindow::SetSize;
  ///@}

  ///@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int, int) override;
  using vtkHardwareWindow::SetPosition;
  ///@}

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor() override;
  void ShowCursor() override;
  void SetCursorPosition(int x, int y) override;
  ///@}

  /**
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) override;

protected:
  vtkWin32HardwareWindow();
  ~vtkWin32HardwareWindow() override;

  HWND ParentId;
  HWND WindowId;
  HINSTANCE ApplicationInstance;

  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  bool CursorHidden;

private:
  vtkWin32HardwareWindow(const vtkWin32HardwareWindow&) = delete;
  void operator=(const vtkWin32HardwareWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
