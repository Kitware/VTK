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
// class vtkOpenGLRenderWindow. It uses Objective-C++, and the OpenGL and
// Cocoa APIs. This class's default behaviour is to create an NSWindow and
// a vtkCocoaGLView which are used together to draw all vtk stuff into.
// If you already have an NSWindow and vtkCocoaGLView and you want this
// class to use them you must call both SetWindowId() and SetDisplayId()
// early on (before WindowInitialize() is executed).
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
  vtkTypeRevisionMacro(vtkCocoaRenderWindow,vtkOpenGLRenderWindow);
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
  virtual void WindowInitialize();

  // Description:
  // Initialize the rendering window.
  virtual void Initialize();

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Remap the window.
  virtual void WindowRemap();

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen();

  // Description:
  // Set the size of the window.
  virtual void SetSize(int*);
  virtual void SetSize(int,int);

  // Description:
  // Get the current size of the window.
  virtual int *GetSize();

  // Description:
  // Set the position of the window.
  virtual void SetPosition(int*);
  virtual void SetPosition(int,int);
  
  // Description:
  // Return the scrren size.
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

  virtual void *GetGenericDisplayId() {return this->NSViewId;}
  virtual void *GetGenericWindowId()  {return this->WindowId;}
  virtual void *GetGenericContext()   {return this->ContextId;}
  
  // Description:
  // Returns the NSView* associated with this vtkRenderWindow.
  virtual void* GetDisplayId();

  // Description:
  // Sets the NSView* associated with this vtkRenderWindow. This class' default
  // behaviour, that is, if you never call this SetDisplayId()/SetWindowId() is
  // to create an NSWindow and a vtkCocoaGLView (NSView subclass) which are used
  // together to draw all vtk stuff into. If you already have an NSWindow and
  // NSView and you want this class to use them you must call both SetWindowId()
  // and SetDisplayId() early on (before WindowInitialize() is executed). In the
  // case of Java, you should call only SetDisplayId().
  virtual void SetDisplayId(void *);
  
  virtual void SetParentId(void *) 
    {
    vtkWarningMacro("Method not implemented.");
    }
  virtual void* GetGenericParentId()
    {
    vtkWarningMacro("Method not implemented.");
    return 0;
    }
  virtual void* GetGenericDrawable()
    {
    vtkWarningMacro("Method not implemented.");
    return 0;
    }
  virtual void SetWindowInfo(char*)
    {
    vtkWarningMacro("Method not implemented.");
    }
  virtual void SetParentInfo(char*)
    {
    vtkWarningMacro("Method not implemented.");
    }

  // Description:
  // Returns the NSWindow* associated with this vtkRenderWindow.
  virtual void *GetWindowId();

  // Description:
  // Sets the NSWindow* associated with this vtkRenderWindow. This class' default
  // behaviour, that is, if you never call this SetDisplayId()/SetWindowId() is
  // to create an NSWindow and a vtkCocoaGLView (NSView subclass) which are used
  // together to draw all vtk stuff into. If you already have an NSWindow and
  // NSView and you want this class to use them you must call both SetWindowId()
  // and SetDisplayId() early on (before WindowInitialize() is executed). In the
  // case of Java, you should call only SetDisplayId().
  virtual void SetWindowId(void *);

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
  // Register a texture name with this render window
  void RegisterTextureResource (GLuint id);

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  virtual void HideCursor();
  virtual void ShowCursor();
  

protected:
  vtkCocoaRenderWindow();
  ~vtkCocoaRenderWindow();

  void CreateGLContext();

private:
  vtkCocoaRenderWindow(const vtkCocoaRenderWindow&);  // Not implemented.
  void operator=(const vtkCocoaRenderWindow&);  // Not implemented.

private:
  void     *ContextId;    // really an NSOpenGLContext*
  void     *WindowId;     // really an NSWindow*
  void     *NSViewId;     // really an NSView* (usually but not necessarily a vtkCocoaGLView*)
  void     *PixelFormat;  // really an NSOpenGLPixelFormat*

  int      WindowCreated;
  int      ViewCreated;
  int      CursorHidden;

  void     *AutoreleasePool; // really an NSAutoreleasePool*
  int      ForceMakeCurrent;
  char     *Capabilities;
};

#endif
