/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXWebGPURenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkXWebGPURenderWindow
 * @brief WebGPU rendering window for the X Window system
 *
 * vtkXWebGPURenderWindow is a concrete implementation of the abstract class vtkRenderWindow.
 * vtkWebGPURenderer interfaces to the OpenGL graphics library.  Application programmers should
 * normally use vtkRenderWindow instead of the WebGPU specific version.
 */

#ifndef vtkXWebGPURenderWindow_h
#define vtkXWebGPURenderWindow_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPURenderWindow.h"
#include <X11/Xlib.h> // Needed for X types used in the public interface

VTK_ABI_NAMESPACE_BEGIN
// Forward declarations

class VTKRENDERINGWEBGPU_EXPORT vtkXWebGPURenderWindow : public vtkWebGPURenderWindow
{
public:
  /**
   * Instantiate the class.
   */
  static vtkXWebGPURenderWindow* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkXWebGPURenderWindow, vtkWebGPURenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * End the rendering process and display the image.
   */
  void Frame() override;

  /**
   * Initialize the window for rendering.
   */
  virtual void WindowInitialize();

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  bool Initialize() override;

  /**
   * "Deinitialize" the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() override;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(vtkTypeBool) override;

  /**
   * Resize the window.
   */
  void WindowRemap() override;

  // Call X funcs to map unmap
  void SetShowWindow(bool val) override;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen();

  /**
   * Set the size (width and height in pixels) of the rendering window.
   * If this is a toplevel window with borders, then the request for a
   * new size is redirected to the window manager. If the window manager
   * chooses a different size for the window, the size it chooses will
   * take effect at the next render, otherwise the size change will take
   * effect immediately. In the rare case that the window manager does
   * does not respond at all (buggy/frozen window manager), the SetSize()
   * method will wait for the response for two seconds before returning.
   *
   * If the size has changed, a vtkCommand::WindowResizeEvent will fire.
   */
  void SetSize(int width, int height) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(vtkTypeBool capable) override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  vtkTypeBool IsDirect() override;

  /**
   * Xwindow get set functions
   */
  void* GetGenericDisplayId() override { return this->GetDisplayId(); }

  void* GetGenericWindowId() override;
  void* GetGenericParentId() override { return reinterpret_cast<void*>(this->ParentId); }

  void* GetGenericDrawable() override { return reinterpret_cast<void*>(this->WindowId); }

  /**
   * Get the current size of the screen in pixels.
   * An HDTV for example would be 1920 x 1080 pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position (x and y) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  /**
   * Get this RenderWindow's X display id.
   */
  Display* GetDisplayId();

  ///@{
  /**
   * Set the X display id for this RenderWindow to use to a pre-existing
   * X display id.
   */
  void SetDisplayId(Display*);
  void SetDisplayId(void*) override;
  ///@}

  /**
   * Get this RenderWindow's parent X window id.
   */
  Window GetParentId();

  ///@{
  /**
   * Sets the parent of the window that WILL BE created.
   */
  void SetParentId(Window);
  void SetParentId(void*) override;
  ///@}

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

  /**
   * Specify the X window id to use if a WindowRemap is done.
   */
  void SetNextWindowId(Window);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void*) override;

  /**
   * Set name of rendering window.
   */
  void SetWindowName(const char*) override;

  /**
   * For window manager that supports it, set the icon displayed
   * in the taskbar and the title bar.
   */
  void SetIcon(vtkImageData* img) override;

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Does this platform support render window data sharing.
   */
  bool GetPlatformSupportsRenderWindowSharing() override { return true; }

  ///@{
  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  void SetPosition(int x, int y) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
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
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  vtkTypeBool GetEventPending() override;

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowInfo(const char* info) override;

  /**
   * Set the window info that will be used after WindowRemap()
   */
  void SetNextWindowInfo(const char* info) override;

  /**
   * Sets the X window id of the window that WILL BE created.
   */
  void SetParentInfo(const char* info) override;

  /**
   * This computes the size of the render window
   * before calling the supper classes render
   */
  void Render() override;

protected:
  vtkXWebGPURenderWindow();
  ~vtkXWebGPURenderWindow() override;

  // Helper members
  Window ParentId;
  Window WindowId;
  Window NextWindowId;
  Display* DisplayId;
  Colormap ColorMap;
  vtkTypeBool OwnWindow;
  vtkTypeBool OwnDisplay;
  vtkTypeBool CursorHidden;
  vtkTypeBool UsingHardware;

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

  std::string MakeDefaultWindowNameWithBackend() override;
  void CreateAWindow() override;
  void DestroyWindow() override;
  void CloseDisplay();

private:
  vtkXWebGPURenderWindow(const vtkXWebGPURenderWindow&) = delete;
  void operator=(const vtkXWebGPURenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkXWebGPURenderWindow_h
