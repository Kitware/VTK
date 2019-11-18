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

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <stack>                       // for ivar

class VTKRENDERINGOPENGL2_EXPORT vtkCocoaRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCocoaRenderWindow* New();
  vtkTypeMacro(vtkCocoaRenderWindow, vtkOpenGLRenderWindow);
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
   * implemented for the vtkCocoaRenderWindow.  It can only be called
   * before the window has been created, and it might not work on all
   * versions of OS X.
   */
  void SetFullScreen(vtkTypeBool) override;

  /**
   * Remap the window.  This is not implemented for the vtkCocoaRenderWindow.
   */
  void WindowRemap() override;

  /**
   * Set the preferred window size to full screen.  This is not implemented
   * for the vtkCocoaRenderWindow.
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
    vtkDebugMacro("SetNextWindowInfo not implemented (WindowRemap not implemented).");
  }
  void* GetGenericDrawable() override
  {
    vtkDebugMacro("Method not implemented.");
    return nullptr;
  }
  void SetDisplayId(void*) override
  {
    // no-op
    vtkDebugMacro("Method not implemented.");
  }
  void* GetGenericDisplayId() override
  {
    vtkDebugMacro("Method not implemented.");
    return nullptr;
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
   * address of the parent NSView.
   */
  void SetParentInfo(const char*) override;

  void SetNextWindowId(void*) override
  {
    vtkDebugMacro("SetNextWindowId not implemented (WindowRemap not implemented).");
  }

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Does this platform support render window data sharing.
   */
  bool GetPlatformSupportsRenderWindowSharing() override { return true; }

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
   * currently only an issue on Mac OS X Cocoa where rendering
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
  void SetContextId(void*);
  void* GetContextId();
  void* GetGenericContext() override { return this->GetContextId(); }
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
  virtual void SetRootWindow(void*);

  /**
   * Returns the NSWindow* associated with this vtkRenderWindow.
   */
  virtual void* GetRootWindow();

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
  void SetWindowId(void*) override;

  /**
   * Returns the NSView* associated with this vtkRenderWindow.
   */
  virtual void* GetWindowId();
  void* GetGenericWindowId() override { return this->GetWindowId(); }

  /**
   * Set the NSView* for the vtkRenderWindow to be parented within.  The
   * Position and Size of the RenderWindow will set the rectangle of the
   * NSView that the vtkRenderWindow will create within this parent.
   * If you set the WindowId, then this ParentId will be ignored.
   */
  void SetParentId(void* nsview) override;

  /**
   * Get the parent NSView* for this vtkRenderWindow.  This method will
   * return "NULL" if the parent was not set with SetParentId() or
   * SetParentInfo().
   */
  virtual void* GetParentId();
  void* GetGenericParentId() override { return this->GetParentId(); }

  /**
   * Set to true if you want to force NSViews created by this object to
   * have their wantsBestResolutionOpenGLSurface property set to YES.
   * Otherwise, the bundle's Info.plist will be checked for the
   * "NSHighResolutionCapable" key, if it is present and YES,
   * wantsBestResolutionOpenGLSurface will be set to YES. In all other cases,
   * setWantsBestResolutionOpenGLSurface: is not invoked at all. Notably,
   * setWantsBestResolutionOpenGLSurface: is never invoked on NSViews not created
   * by VTK itself.
   */
  void SetWantsBestResolution(bool wantsBest);
  bool GetWantsBestResolution();

  //@{
  /**
   * Accessors for the pixel format object (Really an NSOpenGLPixelFormat*).
   */
  void SetPixelFormat(void* pixelFormat);
  void* GetPixelFormat();
  //@}

  //@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  void PushContext() override;
  void PopContext() override;
  //@}

protected:
  vtkCocoaRenderWindow();
  ~vtkCocoaRenderWindow() override;

  std::stack<void*> ContextStack;

  void CreateGLContext();

  void CreateAWindow() override;
  void DestroyWindow() override;
  int OnScreenInitialized;

  //@{
  /**
   * Accessors for the cocoa manager (Really an NSMutableDictionary*).
   * It manages all Cocoa objects in this C++ class.
   */
  void SetCocoaManager(void* manager);
  void* GetCocoaManager();
  //@}

  void SetCocoaServer(void* server); // Really a vtkCocoaServer*
  void* GetCocoaServer();

private:
  vtkCocoaRenderWindow(const vtkCocoaRenderWindow&) = delete;
  void operator=(const vtkCocoaRenderWindow&) = delete;

private:
  // Important: this class cannot contain Objective-C instance
  // variables for 2 reasons:
  // 1) C++ files include this header
  // 2) because of garbage collection (the GC scanner does not scan objects create by C++'s new)
  // Instead, use the CocoaManager dictionary to keep a collection
  // of what would otherwise be Objective-C instance variables.
  void* CocoaManager; // Really an NSMutableDictionary*

  int WindowCreated;
  int ViewCreated;
  int CursorHidden;

  int ForceMakeCurrent;

  bool WantsBestResolution;
};

#endif
