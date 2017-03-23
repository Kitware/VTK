/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCocoaRenderWindow.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCocoaRenderWindow
 * @brief   Cocoa OpenGL rendering window
 *
 *
 * vtkCocoaRenderWindow is a concrete implementation of the abstract
 * class vtkOpenGLRenderWindow. It is only available on Mac OS X.
 * To use this class, build VTK with VTK_USE_COCOA turned ON (this is
 * the default).
 * This class can be used by 32 and 64 bit processes, and either in
 * garbage collected or reference counted modes. ARC is not yet supported.
 * vtkCocoaRenderWindow uses Objective-C++, and the OpenGL and
 * Cocoa APIs. This class's default behaviour is to create an NSWindow and
 * a vtkCocoaGLView which are used together to draw all VTK content.
 * If you already have an NSWindow and vtkCocoaGLView and you want this
 * class to use them you must call both SetRootWindow() and SetWindowId(),
 * respectively, early on (before WindowInitialize() is executed).
 *
 * @sa
 * vtkOpenGLRenderWindow vtkCocoaGLView
 *
 * @warning
 * This header must be in C++ only because it is included by .cxx files.
 * That means no Objective-C may be used. That's why some instance variables
 * are void* instead of what they really should be.
*/

#ifndef vtkCocoaRenderWindow_h
#define vtkCocoaRenderWindow_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

class VTKRENDERINGOPENGL_EXPORT vtkCocoaRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCocoaRenderWindow *New();
  vtkTypeMacro(vtkCocoaRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Begin the rendering process.
   */
  void Start() VTK_OVERRIDE;

  /**
   * Finish the rendering process.
   */
  void Frame() VTK_OVERRIDE;

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
  virtual void Initialize();

  /**
   * Change the window to fill the entire screen.  This is only partially
   * implemented for the vtkCocoaRenderWindow.  It can only be called
   * before the window has been created, and it might not work on all
   * versions of OS X.
   */
  void SetFullScreen(int) VTK_OVERRIDE;

  /**
   * Remap the window.  This is not implemented for the vtkCocoaRenderWindow.
   */
  void WindowRemap() VTK_OVERRIDE;

  /**
   * Set the preferred window size to full screen.  This is not implemented
   * for the vtkCocoaRenderWindow.
   */
  virtual void PrefFullScreen();

  //@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int a[2]) VTK_OVERRIDE;
  void SetSize(int,int) VTK_OVERRIDE;
  //@}

  /**
   * Get the current size of the window in pixels.
   */
  int *GetSize() VTK_OVERRIDE;

  //@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int a[2]) VTK_OVERRIDE;
  void SetPosition(int,int) VTK_OVERRIDE;
  //@}

  /**
   * Get the current size of the screen in pixels.
   */
  int *GetScreenSize() VTK_OVERRIDE;

  /**
   * Get the position in screen coordinates of the window.
   */
  int *GetPosition() VTK_OVERRIDE;

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char *) VTK_OVERRIDE;

  void SetNextWindowInfo(char *) VTK_OVERRIDE
  {
      vtkWarningMacro("SetNextWindowInfo not implemented (WindowRemap not implemented).");
  }
  void* GetGenericDrawable() VTK_OVERRIDE
  {
      vtkWarningMacro("Method not implemented.");
      return 0;
  }
  void SetDisplayId(void*) VTK_OVERRIDE
  {
      vtkWarningMacro("Method not implemented.");
  }
  void *GetGenericDisplayId() VTK_OVERRIDE
  {
      vtkWarningMacro("Method not implemented.");
      return 0;
  }

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   * The paramater is an ASCII string of a decimal number representing
   * a pointer to the window.
   */
  void SetWindowInfo(char*) VTK_OVERRIDE;

  /**
   * See the documenation for SetParentId().  This method allows the ParentId
   * to be set as an ASCII string of a decimal number that is the memory
   * address of the parent NSView.
   */
  void SetParentInfo(char*) VTK_OVERRIDE;

  void SetNextWindowId(void*) VTK_OVERRIDE
  {
      vtkWarningMacro("SetNextWindowId not implemented (WindowRemap not implemented).");
  }

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() VTK_OVERRIDE;

  /**
   * Update system if needed due to stereo rendering.
   */
  void StereoUpdate() VTK_OVERRIDE;

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(int capable) VTK_OVERRIDE;

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent() VTK_OVERRIDE;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() VTK_OVERRIDE;

  /**
   * Test if the window has a valid drawable. This is
   * currently only an issue on Mac OS X Cocoa where rendering
   * to an invalid drawable results in all OpenGL calls to fail
   * with "invalid framebuffer operation".
   */
  bool IsDrawable() VTK_OVERRIDE;

  /**
   * Update this window's OpenGL context, e.g. when the window is resized.
   */
  void UpdateContext();

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities() VTK_OVERRIDE;

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() VTK_OVERRIDE;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() VTK_OVERRIDE;

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  void SetForceMakeCurrent() VTK_OVERRIDE;

  /**
   * Check to see if an event is pending for this window.
   * This is a useful check to abort a long render.
   */
   int GetEventPending() VTK_OVERRIDE;

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, void *dwFlags, int debug,
                                int bpp=16, int zbpp=16);
  //@}

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Finalize() VTK_OVERRIDE;

  /**
   * Get the size of the depth buffer.
   */
  int GetDepthBufferSize() VTK_OVERRIDE;

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor() VTK_OVERRIDE;
  void ShowCursor() VTK_OVERRIDE;
  void SetCursorPosition(int x, int y) VTK_OVERRIDE;
  //@}

  /**
   * Change the shape of the cursor.
   */
  void SetCurrentCursor(int) VTK_OVERRIDE;

  /**
   * Get the ViewCreated flag. It is 1 if this object created an instance
   * of NSView, 0 otherwise.
   */
  virtual int GetViewCreated();

  /**
   * Get the WindowCreated flag. It is 1 if this object created an instance
   * of NSWindow, 0 otherwise.
   */
  virtual int GetWindowCreated();

  //@{
  /**
   * Accessors for the OpenGL context (Really an NSOpenGLContext*).
   */
  void SetContextId(void *);
  void *GetContextId();
  void *GetGenericContext() VTK_OVERRIDE   {return this->GetContextId();}
  //@}

  /**
   * Sets the NSWindow* associated with this vtkRenderWindow.
   * This class' default behaviour, that is, if you never call
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkCocoaGLView (NSView subclass) which are used together to draw
   * all vtk stuff into. If you already have an NSWindow and NSView and
   * you want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  virtual void SetRootWindow(void *);

  /**
   * Returns the NSWindow* associated with this vtkRenderWindow.
   */
  virtual void *GetRootWindow();

  /**
   * Sets the NSView* associated with this vtkRenderWindow.
   * This class' default behaviour, that is, if you never call this
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkCocoaGLView (NSView subclass) which are used together to draw all
   * vtk stuff into. If you already have an NSWindow and NSView and you
   * want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  void SetWindowId(void *) VTK_OVERRIDE;

  /**
   * Returns the NSView* associated with this vtkRenderWindow.
   */
  virtual void *GetWindowId();
  void *GetGenericWindowId() VTK_OVERRIDE {return this->GetWindowId();}

  /**
   * Set the NSView* for the vtkRenderWindow to be parented within.  The
   * Position and Size of the RenderWindow will set the rectangle of the
   * NSView that the vtkRenderWindow will create within this parent.
   * If you set the WindowId, then this ParentId will be ignored.
   */
  void SetParentId(void *nsview) VTK_OVERRIDE;

  /**
   * Get the parent NSView* for this vtkRenderWindow.  This method will
   * return "NULL" if the parent was not set with SetParentId() or
   * SetParentInfo().
   */
  virtual void *GetParentId();
  void *GetGenericParentId() VTK_OVERRIDE { return this->GetParentId(); }

  /**
   * Whenever an NSView is created, this value will be passed to
   * setWantsBestResolutionOpenGLSurface:. The default is true.
   * On non-retina displays, this setting has no effect. On retina displays,
   * when true, the full resolution is used, resulting in crisper images
   * at the cost of more memory and processing. When false, the images are
   * magnified instead. setWantsBestResolutionOpenGLSurface: is never invoked
   * on NSViews not created by VTK itself (but merely given to it).
   */
  void SetWantsBestResolution(bool wantsBest);
  bool GetWantsBestResolution();

  //@{
  /**
   * Accessors for the pixel format object (Really an NSOpenGLPixelFormat*).
   */
  void SetPixelFormat(void *pixelFormat);
  void *GetPixelFormat();
  //@}

protected:
  vtkCocoaRenderWindow();
  ~vtkCocoaRenderWindow() VTK_OVERRIDE;

  void CreateGLContext();

  void CreateAWindow() VTK_OVERRIDE;
  void DestroyWindow() VTK_OVERRIDE;
  void DestroyOffScreenWindow();

  int OffScreenInitialized;
  int OnScreenInitialized;

  //@{
  /**
   * Accessors for the cocoa manager (Really an NSMutableDictionary*).
   * It manages all Cocoa objects in this C++ class.
   */
  void SetCocoaManager(void *manager);
  void *GetCocoaManager();
  //@}

  void SetCocoaServer(void *server);            // Really a vtkCocoaServer*
  void *GetCocoaServer();

private:
  vtkCocoaRenderWindow(const vtkCocoaRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCocoaRenderWindow&) VTK_DELETE_FUNCTION;

private:
  // Important: this class cannot contain Objective-C instance
  // variables for 2 reasons:
  // 1) C++ files include this header
  // 2) because of garbage collection (the GC scanner does not scan objects create by C++'s new)
  // Instead, use the CocoaManager dictionary to keep a collection
  // of what would otherwise be Objective-C instance variables.
  void     *CocoaManager; // Really an NSMutableDictionary*

  int      WindowCreated;
  int      ViewCreated;
  int      CursorHidden;

  int      ForceMakeCurrent;
  char     *Capabilities;

  bool     WantsBestResolution;
};

#endif
