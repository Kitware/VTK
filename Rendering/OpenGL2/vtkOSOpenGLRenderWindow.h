// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSOpenGLRenderWindow
 * @brief   OffScreen Mesa rendering window
 *
 * vtkOSOpenGLRenderWindow is a concrete implementation of the abstract class
 * vtkOpenGLRenderWindow. vtkOSOpenGLRenderWindow interfaces to the OffScreen
 * Mesa software implementation of the OpenGL library. The framebuffer resides
 * on host memory. The framebuffer is the collection of logical buffers
 * (color buffer(s), depth buffer, stencil buffer, accumulation buffer,
 * multisample buffer) defining where the output of GL rendering is directed.
 * Application programmers should normally use vtkRenderWindow instead of the
 * OpenGL specific version.
 */

#ifndef vtkOSOpenGLRenderWindow_h
#define vtkOSOpenGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkOSOpenGLRenderWindowInternal;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOSOpenGLRenderWindow
  : public vtkOpenGLRenderWindow
{
public:
  static vtkOSOpenGLRenderWindow* New();
  vtkTypeMacro(vtkOSOpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  void Initialize() override;

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

  ///@{
  /**
   * Set the size of the window in screen coordinates in pixels.
   * This resizes the operating system's window and redraws it.
   *
   * If the size has changed, this method will fire
   * vtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

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

  ///@{
  /**
   * Move the window to a new position on the display.
   */
  void SetPosition(int x, int y) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
  ///@}

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(vtkTypeBool capable) override;

  /**
   * Make this window the current OpenGL context.
   */
  void MakeCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  void SetForceMakeCurrent() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  vtkTypeBool IsDirect() override;

  /**
   * Resize the window.
   */
  void WindowRemap() override;

  ///@{
  /**
   * Xwindow get set functions
   */
  void* GetGenericDisplayId() override { return nullptr; }
  void* GetGenericWindowId() override;
  void* GetGenericParentId() override { return nullptr; }
  void* GetGenericContext() override;
  void* GetGenericDrawable() override { return nullptr; }
  ///@}

  /**
   * Set the X display id for this RenderWindow to use to a pre-existing
   * X display id.
   */
  void SetDisplayId(void*) override {}

  /**
   * Sets the parent of the window that WILL BE created.
   */
  void SetParentId(void*) override;

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowId(void*) override;

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void*) override;

  void SetWindowName(const char*) override;

  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() override {}
  void ShowCursor() override {}

  /**
   * Check to see if a mouse button has been pressed.
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

protected:
  vtkOSOpenGLRenderWindow();
  ~vtkOSOpenGLRenderWindow() override;

  vtkOSOpenGLRenderWindowInternal* Internal;

  vtkTypeBool OwnWindow;
  vtkTypeBool OwnDisplay;
  vtkTypeBool CursorHidden;
  vtkTypeBool ForceMakeCurrent;

  void CreateAWindow() override;
  void DestroyWindow() override;
  void CreateOffScreenWindow(int width, int height);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int width, int height);

private:
  vtkOSOpenGLRenderWindow(const vtkOSOpenGLRenderWindow&) = delete;
  void operator=(const vtkOSOpenGLRenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
