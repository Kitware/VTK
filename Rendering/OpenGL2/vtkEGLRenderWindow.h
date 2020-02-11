/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEGLRenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkEGLRenderWindow is a concrete implementation of the abstract class
 * vtkRenderWindow. This class creates a window on Android platform and
 * for client API OpenGL ES and an offscreen pbuffer for OpenGL.
 * vtkOpenGLRenderer interfaces to the OpenGL graphics library.
 * Application programmers should normally use vtkRenderWindow instead of
 * the OpenGL specific version.
 *
 * If the VTK_DEFAULT_EGL_DEVICE_INDEX environment variable is present at
 * the time of construction, it's value will be used to initialize the
 * DeviceIndex, falling back to the VTK_DEFAULT_EGL_DEVICE_INDEX
 * preprocessor definition otherwise.
 */

#ifndef vtkEGLRenderWindow_h
#define vtkEGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class vtkIdList;

class VTKRENDERINGOPENGL2_EXPORT vtkEGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkEGLRenderWindow* New();
  vtkTypeMacro(vtkEGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * End the rendering process and display the image.
   */
  virtual void Frame(void) override;

  // override as some EGL systems cannot show the window
  void SetShowWindow(bool) override;

  /**
   * Initialize the window for rendering.
   */
  virtual void WindowInitialize(void);

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize(void) override;

  /**
   * "Deinitialize" the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  virtual void Finalize(void) override;

  /**
   * Change the window to fill the entire screen.
   */
  virtual void SetFullScreen(vtkTypeBool) override;

  /**
   * Resize the window.
   */
  virtual void WindowRemap(void) override;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen(void);

  /**
   * Specify the size of the rendering window in pixels.
   */
  virtual void SetSize(int, int) override;
  virtual void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  virtual void SetStereoCapableWindow(vtkTypeBool capable) override;

  /**
   * Make this window the current OpenGL context.
   */
  void MakeCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  virtual bool IsCurrent() override;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() override { return 1; }

  /**
   * Get the current size of the screen in pixels.
   */
  virtual int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates (pixels) of the window.
   */
  virtual int* GetPosition() VTK_SIZEHINT(2) override;

  //@{
  /**
   * Dummy stubs for vtkWindow API.
   */
  virtual void SetDisplayId(void*) override {}
  virtual void SetWindowId(void* window) override;
  virtual void SetNextWindowId(void*) override {}
  virtual void SetParentId(void*) override {}
  virtual void* GetGenericDisplayId() override;
  virtual void* GetGenericWindowId() override { return nullptr; }
  virtual void* GetGenericParentId() override { return nullptr; }
  virtual void* GetGenericContext() override;
  virtual void* GetGenericDrawable() override { return nullptr; }
  virtual void SetWindowInfo(const char*) override;
  virtual void SetNextWindowInfo(const char*) override {}
  virtual void SetParentInfo(const char*) override {}
  //@}

  void SetWindowName(const char*) override;

  //@{
  /**
   * Move the window to a new position on the display.
   */
  void SetPosition(int, int) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
  //@}

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() override;
  void ShowCursor() override;
  //@}

  /**
   * This computes the size of the render window
   * before calling the supper classes render
   */
  void Render() override;

  /**
   * Check to see if a mouse button has been pressed.  All other events
   * are ignored by this method.  Ideally, you want to abort the render
   * on any event which causes the DesiredUpdateRate to switch from
   * a high-quality rate to a more interactive rate.
   */
  virtual int GetEventPending() override { return 0; }

  int GetOwnWindow() { return this->OwnWindow; }

  /**
   * Returns the width and height of the allocated EGL surface.
   * If no surface is allocated width and height are set to 0.
   */
  void GetEGLSurfaceSize(int* width, int* height);
  /**
   * Returns the number of devices (graphics cards) on a system.
   */
  int GetNumberOfDevices() override;
  /**
   * Returns true if driver has an
   * EGL/OpenGL bug that makes vtkChartsCoreCxx-TestChartDoubleColors and other tests to fail
   * because point sprites don't work correctly (gl_PointCoord is undefined) unless
   * glEnable(GL_POINT_SPRITE)
   */
  bool IsPointSpriteBugPresent() override;

protected:
  vtkEGLRenderWindow();
  ~vtkEGLRenderWindow() override;

  int ScreenSize[2];
  int OwnWindow;
  bool IsPointSpriteBugTested;
  bool IsPointSpriteBugPresent_;
  class vtkInternals;
  vtkInternals* Internals;

  void CreateAWindow() override;
  void DestroyWindow() override;
  void ResizeWindow(int width, int height);

  /**
   * Use EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base
   * extensions to set the display (output graphics card) to something different than
   * EGL_DEFAULT_DISPLAY. Just use the default display if deviceIndex == 0.
   */
  void SetDeviceAsDisplay(int deviceIndex);

private:
  vtkEGLRenderWindow(const vtkEGLRenderWindow&) = delete;
  void operator=(const vtkEGLRenderWindow&) = delete;

  bool DeviceExtensionsPresent;
};

#endif
