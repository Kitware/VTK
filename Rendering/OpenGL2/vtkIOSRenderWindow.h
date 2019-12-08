/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkIOSRenderWindow.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIOSRenderWindow
 * @brief   IOS OpenGL rendering window
 *
 *
 * vtkIOSRenderWindow is a concrete implementation of the abstract
 * class vtkOpenGLRenderWindow. It is only available on iOS.
 * To use this class, build VTK with VTK_USE_IOS turned ON (this is
 * the default).
 * This class can be used by 32 and 64 bit processes, and either in
 * garbage collected or reference counted modes. ARC is not supported.
 * vtkIOSRenderWindow uses Objective-C++, and the OpenGL and
 * IOS APIs. This class's default behaviour is to create an NSWindow and
 * a vtkIOSGLView which are used together to draw all VTK content.
 * If you already have an NSWindow and vtkIOSGLView and you want this
 * class to use them you must call both SetRootWindow() and SetWindowId(),
 * respectively, early on (before WindowInitialize() is executed).
 *
 * @sa
 * vtkOpenGLRenderWindow vtkIOSGLView
 *
 * @warning
 * This header must be in C++ only because it is included by .cxx files.
 * That means no Objective-C may be used. That's why some instance variables
 * are void* instead of what they really should be.
 */

#ifndef vtkIOSRenderWindow_h
#define vtkIOSRenderWindow_h

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class VTKRENDERINGOPENGL2_EXPORT vtkIOSRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkIOSRenderWindow* New();
  vtkTypeMacro(vtkIOSRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Begin the rendering process.
   */
  void Start() override;

  /**
   * Finish the rendering process.
   */
  void Frame() override;

  /**
   * Specify various window parameters.
   */
  virtual void WindowConfigure();

  /**
   * Initialize the window for rendering.
   * virtual void WindowInitialize();
   */

  /**
   * Initialize the rendering window.
   */
  void Initialize() override;

  /**
   * Change the window to fill the entire screen.  This is only partially
   * implemented for the vtkIOSRenderWindow.  It can only be called
   * before the window has been created, and it might not work on all
   * versions of OS X.
   */
  void SetFullScreen(vtkTypeBool) override;

  /**
   * Remap the window.  This is not implemented for the vtkIOSRenderWindow.
   */
  void WindowRemap() override;

  /**
   * Set the preferred window size to full screen.  This is not implemented
   * for the vtkIOSRenderWindow.
   */
  virtual void PrefFullScreen();

  //@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int a[2]) override;
  void SetSize(int, int) override;
  //@}

  /**
   * Get the current size of the window in pixels.
   */
  int* GetSize() VTK_SIZEHINT(2) override;

  //@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int a[2]) override;
  void SetPosition(int, int) override;
  //@}

  /**
   * Get the current size of the screen in pixels.
   */
  int* GetScreenSize() VTK_SIZEHINT(2) override;

  /**
   * Get the position in screen coordinates of the window.
   */
  int* GetPosition() VTK_SIZEHINT(2) override;

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char*) override;

  void SetNextWindowInfo(const char*) override
  {
    vtkWarningMacro("SetNextWindowInfo not implemented (WindowRemap not implemented).");
  }
  void* GetGenericDrawable() override
  {
    vtkWarningMacro("Method not implemented.");
    return 0;
  }
  void SetDisplayId(void*) override { vtkWarningMacro("Method not implemented."); }
  void* GetGenericDisplayId() override
  {
    vtkWarningMacro("Method not implemented.");
    return 0;
  }

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   * The parameter is an ASCII string of a decimal number representing
   * a pointer to the window.
   */
  void SetWindowInfo(const char*) override;

  /**
   * See the documentation for SetParentId().  This method allows the ParentId
   * to be set as an ASCII string of a decimal number that is the memory
   * address of the parent UIView.
   */
  void SetParentInfo(const char*) override;

  void SetNextWindowId(void*) override
  {
    vtkWarningMacro("SetNextWindowId not implemented (WindowRemap not implemented).");
  }

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(vtkTypeBool capable) override;

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() override;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() override;

  /**
   * Test if the window has a valid drawable. This is
   * currently only an issue on Mac OS X IOS where rendering
   * to an invalid drawable results in all OpenGL calls to fail
   * with "invalid framebuffer operation".
   */
  bool IsDrawable() override;

  /**
   * Update this window's OpenGL context, e.g. when the window is resized.
   */
  void UpdateContext();

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
  int IsDirect() override;

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  void SetForceMakeCurrent() override;

  /**
   * Check to see if an event is pending for this window.
   * This is a useful check to abort a long render.
   */
  int GetEventPending() override;

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(void* hDC);
  virtual void SetupPixelFormat(void* hDC, void* dwFlags, int debug, int bpp = 16, int zbpp = 16);
  //@}

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Finalize() override;

  /**
   * Get the size of the depth buffer.
   */
  int GetDepthBufferSize() override;

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor() override;
  void ShowCursor() override;
  void SetCursorPosition(int x, int y) override;
  //@}

  /**
   * Change the shape of the cursor.
   */
  void SetCurrentCursor(int) override;

  /**
   * Get the WindowCreated flag. It is 1 if this object created an instance
   * of NSWindow, 0 otherwise.
   */
  virtual int GetWindowCreated();

  //@{
  /**
   * Accessors for the OpenGL context (Really an NSOpenGLContext*).
   */
  void SetContextId(void*);
  void* GetContextId();
  void* GetGenericContext() override { return this->GetContextId(); }
  //@}

  /**
   * Sets the NSWindow* associated with this vtkRenderWindow.
   * This class' default behaviour, that is, if you never call
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkIOSGLView (UIView subclass) which are used together to draw
   * all vtk stuff into. If you already have an NSWindow and UIView and
   * you want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  virtual void SetRootWindow(void*);

  /**
   * Returns the NSWindow* associated with this vtkRenderWindow.
   */
  virtual void* GetRootWindow();

  /**
   * Sets the UIView* associated with this vtkRenderWindow.
   * This class' default behaviour, that is, if you never call this
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkIOSGLView (UIView subclass) which are used together to draw all
   * vtk stuff into. If you already have an NSWindow and UIView and you
   * want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  void SetWindowId(void*) override;

  /**
   * Returns the UIView* associated with this vtkRenderWindow.
   */
  virtual void* GetWindowId();
  void* GetGenericWindowId() override { return this->GetWindowId(); }

  /**
   * Set the UIView* for the vtkRenderWindow to be parented within.  The
   * Position and Size of the RenderWindow will set the rectangle of the
   * UIView that the vtkRenderWindow will create within this parent.
   * If you set the WindowId, then this ParentId will be ignored.
   */
  void SetParentId(void* UIView) override;

  /**
   * Get the parent UIView* for this vtkRenderWindow.  This method will
   * return "NULL" if the parent was not set with SetParentId() or
   * SetParentInfo().
   */
  virtual void* GetParentId();
  void* GetGenericParentId() override { return this->GetParentId(); }

  //@{
  /**
   * Accessors for the pixel format object (Really an NSOpenGLPixelFormat*).
   */
  void SetPixelFormat(void* pixelFormat);
  void* GetPixelFormat();
  //@}

protected:
  vtkIOSRenderWindow();
  ~vtkIOSRenderWindow() override;

  void CreateGLContext();

  void CreateAWindow() override;
  void DestroyWindow() override;
  void DestroyOffScreenWindow();

  int OffScreenInitialized;
  int OnScreenInitialized;

  // IOS seems to have issues with getting RGB data
  int ReadPixels(
    const vtkRecti& rect, int front, int glFormat, int glType, void* data, int right = 0) override;

private:
  vtkIOSRenderWindow(const vtkIOSRenderWindow&) = delete;
  void operator=(const vtkIOSRenderWindow&) = delete;

private:
  int WindowCreated;
  int ViewCreated;
  int CursorHidden;

  int ForceMakeCurrent;
};

#endif
