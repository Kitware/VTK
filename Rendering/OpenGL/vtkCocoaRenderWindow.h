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
// .NAME vtkCocoaRenderWindow - Cocoa OpenGL rendering window
//
// .SECTION Description
// vtkCocoaRenderWindow is a concrete implementation of the abstract
// class vtkOpenGLRenderWindow. It is only available on Mac OS X 10.3
// and later.
// To use this class, build VTK with VTK_USE_COCOA turned ON.
// This class can be used by 32 and 64 bit processes, and either in
// garbage collected or reference counted modes.
// vtkCocoaRenderWindow uses Objective-C++, and the OpenGL and
// Cocoa APIs. This class's default behaviour is to create an NSWindow and
// a vtkCocoaGLView which are used together to draw all vtk stuff into.
// If you already have an NSWindow and vtkCocoaGLView and you want this
// class to use them you must call both SetRootWindow() and SetWindowId(),
// respectively, early on (before WindowInitialize() is executed).
//
// .SECTION See Also
// vtkOpenGLRenderWindow vtkCocoaGLView

// .SECTION Warning
// This header must be in C++ only because it is included by .cxx files.
// That means no Objective-C may be used. That's why some instance variables 
// are void* instead of what they really should be.

#ifndef __vtkCocoaRenderWindow_h
#define __vtkCocoaRenderWindow_h

#include "vtkOpenGLRenderWindow.h"

class VTK_RENDERING_EXPORT vtkCocoaRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCocoaRenderWindow *New();
  vtkTypeMacro(vtkCocoaRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start();

  // Description:
  // Finish the rendering process.
  virtual void Frame();

  // Description:
  // Specify various window parameters.
  virtual void WindowConfigure();

  // Description:
  // Initialize the window for rendering.
  //  virtual void WindowInitialize();

  // Description:
  // Initialize the rendering window.
  virtual void Initialize();

  // Description:
  // Change the window to fill the entire screen.  This is only partially
  // implemented for the vtkCocoaRenderWindow.  It can only be called
  // before the window has been created, and it might not work on all
  // versions of OS X.
  virtual void SetFullScreen(int);

  // Description:
  // Remap the window.  This is not implemented for the vtkCocoaRenderWindow.
  virtual void WindowRemap();

  // Description:
  // Set the preferred window size to full screen.  This is not implemented
  // for the vtkCocoaRenderWindow.
  virtual void PrefFullScreen();

  // Description:
  // Set the size of the window in pixels.
  virtual void SetSize(int a[2]);
  virtual void SetSize(int,int);

  // Description:
  // Get the current size of the window in pixels.
  virtual int *GetSize();

  // Description:
  // Set the position of the window.
  virtual void SetPosition(int a[2]);
  virtual void SetPosition(int,int);
  
  // Description:
  // Get the current size of the screen in pixels.
  virtual int *GetScreenSize();

  // Description:
  // Get the position in screen coordinates of the window.
  virtual int *GetPosition();

  // Description:
  // Set the name of the window. This appears at the top of the window
  // normally.
  virtual void SetWindowName(const char *);
  
  void SetNextWindowInfo(char *)
    {
      vtkWarningMacro("SetNextWindowInfo not implemented (WindowRemap not implemented).");
    }
  virtual void* GetGenericDrawable()
    {
      vtkWarningMacro("Method not implemented.");
      return 0;
    }
  virtual void SetDisplayId(void*)
    {
      vtkWarningMacro("Method not implemented.");
    }
  virtual void *GetGenericDisplayId()
    {
      vtkWarningMacro("Method not implemented.");
      return 0;
    }

  // Description:
  // Set this RenderWindow's window id to a pre-existing window.
  // The paramater is an ASCII string of a decimal number representing
  // a pointer to the window.
  virtual void SetWindowInfo(char*);

  // Description:
  // See the documenation for SetParentId().  This method allows the ParentId
  // to be set as an ASCII string of a decimal number that is the memory
  // address of the parent NSView.
  virtual void SetParentInfo(char*);

  void SetNextWindowId(void*)
    {
      vtkWarningMacro("SetNextWindowId not implemented (WindowRemap not implemented).");
    }


  // Description:
  // Update system if needed due to stereo rendering.
  virtual void StereoUpdate();
  
  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this windows OpenGL context the current context.
  virtual void MakeCurrent();
  
  // Description:
  // Tells if this window is the current OpenGL context for the calling thread.
  virtual bool IsCurrent();
  
  // Description:
  // Update this window's OpenGL context, e.g. when the window is resized.
  void UpdateContext();

  // Description:
  // Get report of capabilities for the render window
  const char *ReportCapabilities();

  // Description:
  // Does this render window support OpenGL? 0-false, 1-true
  int SupportsOpenGL();

  // Description:      
  // Is this render window using hardware acceleration? 0-false, 1-true   
  int IsDirect();

  // Description:
  // If called, allow MakeCurrent() to skip cache-check when called.
  // MakeCurrent() reverts to original behavior of cache-checking
  // on the next render.
  virtual void SetForceMakeCurrent();

  // Description:
  // Check to see if an event is pending for this window.
  // This is a useful check to abort a long render.
  virtual  int GetEventPending();

  // Description:
  // Initialize OpenGL for this window.
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, void *dwFlags, int debug, 
                                int bpp=16, int zbpp=16);
  
  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Finalize();

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  // Set cursor position in window (note that (0,0) is the lower left 
  // corner).
  virtual void HideCursor();
  virtual void ShowCursor();
  virtual void SetCursorPosition(int x, int y);

  // Description:
  // Change the shape of the cursor.
  virtual void SetCurrentCursor(int);
  
  // Description:
  // Get the WindowCreated flag. It is 1 if this object created an instance
  // of NSWindow, 0 otherwise.
  virtual int GetWindowCreated();
  
  // Description:
  // Accessors for the OpenGL context (Really an NSOpenGLContext*).
  void SetContextId(void *);
  void *GetContextId();
  virtual void *GetGenericContext()   {return this->GetContextId();}

  // Description:
  // Sets the NSWindow* associated with this vtkRenderWindow.
  // This class' default behaviour, that is, if you never call
  // SetWindowId()/SetRootWindow() is to create an NSWindow and a
  // vtkCocoaGLView (NSView subclass) which are used together to draw
  // all vtk stuff into. If you already have an NSWindow and NSView and
  // you want this class to use them you must call both SetRootWindow()
  // and SetWindowId(), respectively, early on (before WindowInitialize()
  // is executed). In the case of Java, you should call only SetWindowId().
  virtual void SetRootWindow(void *);
  
  // Description:
  // Returns the NSWindow* associated with this vtkRenderWindow.
  virtual void *GetRootWindow();

  // Description:
  // Sets the NSView* associated with this vtkRenderWindow.
  // This class' default behaviour, that is, if you never call this
  // SetWindowId()/SetRootWindow() is to create an NSWindow and a
  // vtkCocoaGLView (NSView subclass) which are used together to draw all
  // vtk stuff into. If you already have an NSWindow and NSView and you
  // want this class to use them you must call both SetRootWindow()
  // and SetWindowId(), respectively, early on (before WindowInitialize()
  // is executed). In the case of Java, you should call only SetWindowId().
  virtual void SetWindowId(void *);

  // Description:
  // Returns the NSView* associated with this vtkRenderWindow.
  virtual void *GetWindowId();
  virtual void *GetGenericWindowId() {return this->GetWindowId();}
  
  // Description:
  // Set the NSView* for the vtkRenderWindow to be parented within.  The
  // Position and Size of the RenderWindow will set the rectangle of the
  // NSView that the vtkRenderWindow will create within this parent.
  // If you set the WindowId, then this ParentId will be ignored.
  virtual void SetParentId(void *nsview);

  // Description:
  // Get the parent NSView* for this vtkRenderWindow.  This method will
  // return "NULL" if the parent was not set with SetParentId() or 
  // SetParentInfo().
  virtual void *GetParentId();
  virtual void *GetGenericParentId() { return this->GetParentId(); }

  // Description:
  // Returns the scaling factor for 'resolution independence', to convert
  // between points and pixels.
  vtkGetMacro(ScaleFactor, double);
  
  // Description:
  // Accessors for the pixel format object (Really an NSOpenGLPixelFormat*).
  void SetPixelFormat(void *pixelFormat);
  void *GetPixelFormat();

protected:
  vtkCocoaRenderWindow();
  ~vtkCocoaRenderWindow();

  void CreateGLContext();

  void CreateAWindow();
  void DestroyWindow();
  void DestroyOffScreenWindow();

  int OffScreenInitialized;
  int OnScreenInitialized;
  
  // Using CGFloat would be better, but doing it this way avoids pulling in
  // Apple headers, which cause problems with the 10.3 SDK and python wrappings.
#if defined(__LP64__) && __LP64__
  double ScaleFactor;
#else
  float ScaleFactor;
#endif

  // Description:
  // Accessors for the cocoa manager (Really an NSMutableDictionary*).
  // It manages all Cocoa objects in this C++ class.
  void SetCocoaManager(void *manager);
  void *GetCocoaManager();

private:
  vtkCocoaRenderWindow(const vtkCocoaRenderWindow&);  // Not implemented.
  void operator=(const vtkCocoaRenderWindow&);  // Not implemented.

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
};

#endif
