/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXlibHardwareWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXlibHardwareWindow
 * @brief   represents a window in a windows GUI
 */

#ifndef vtkXlibHardwareWindow_h
#define vtkXlibHardwareWindow_h

#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro

#include <X11/Xlib.h>  // Needed for X types used in the public interface
#include <X11/Xutil.h> // Needed for X types used in the public interface

VTK_ABI_NAMESPACE_BEGIN
// Forward declarations
class vtkImageData;

class VTKRENDERINGUI_EXPORT vtkXlibHardwareWindow : public vtkHardwareWindow
{
public:
  static vtkXlibHardwareWindow* New();
  vtkTypeMacro(vtkXlibHardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the X11 display.
   *
   * If unset, windows will open the default display (":0.0" unless
   * the DISPLAY environment variable is provided).
   */
  Display* GetDisplayId();
  void SetDisplayId(Display* display);
  ///@}

  /**
   * Sets the parent of the window that WILL BE created.
   */
  void SetParentId(Window);

  /**
   * Get this RenderWindow's X window id.
   */
  Window GetWindowId();

  ///@{
  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowId(Window);
  void SetWindowId(void*) override;
  ///@}

  void Create() override;
  void Destroy() override;

  ///@{
  /**
   * These are window system independent methods that are used
   * to help interface vtkWindow to native windowing systems.
   */
  void SetDisplayId(void*) override;
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
  /*
   * Set/get whether the window should be fullscreen or not.
   */
  vtkGetMacro(FullScreen, vtkTypeBool);
  vtkSetMacro(FullScreen, vtkTypeBool);
  vtkBooleanMacro(FullScreen, vtkTypeBool);
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
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) override;

  /**
   * Set name of window.
   */
  void SetWindowName(const char*) override;

  /**
   * For window manager that supports it, set the icon displayed
   * in the taskbar and the title bar.
   */
  void SetIcon(vtkImageData* img) override;

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowInfo(const char* info) override;

protected:
  vtkXlibHardwareWindow();
  ~vtkXlibHardwareWindow() override;

  // Helper members
  XVisualInfo* GetDesiredVisualInfo();
  vtkTypeBool OpenDisplay();
  void CloseDisplay();

  Display* DisplayId;
  Window ParentId;
  Window WindowId;
  Colormap ColorMap;
  vtkTypeBool OwnDisplay;
  vtkTypeBool OwnWindow;
  vtkTypeBool FullScreen;
  vtkTypeBool CursorHidden;

  // we must keep track of the cursors we are using
  Cursor XCCrosshair;
  Cursor XCArrow;
  Cursor XCSizeAll;
  Cursor XCSizeNS;
  Cursor XCSizeWE;
  Cursor XCSizeNE;
  Cursor XCSizeNW;
  Cursor XCSizeSE;
  Cursor XCSizeSW;
  Cursor XCHand;
  Cursor XCCustom;

private:
  vtkXlibHardwareWindow(const vtkXlibHardwareWindow&) = delete;
  void operator=(const vtkXlibHardwareWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkXlibHardwareWindow_h
