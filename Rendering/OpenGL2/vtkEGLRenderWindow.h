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
// .NAME vtkEGLRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkEGLRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. This class creates a window on Android platform and for client API OpenGL ES and
// an offscreen pbuffer for OpenGL.
// vtkOpenGLRenderer interfaces to the OpenGL graphics library.
// Application programmers should normally use vtkRenderWindow instead of the OpenGL specific version.

#ifndef vtkEGLRenderWindow_h
#define vtkEGLRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

class vtkIdList;

class VTKRENDERINGOPENGL2_EXPORT vtkEGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkEGLRenderWindow *New();
  vtkTypeMacro(vtkEGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // End the rendering process and display the image.
  virtual void Frame(void);

  // Description:
  // Initialize the window for rendering.
  virtual void WindowInitialize(void);

  // Description:
  // Initialize the rendering window.  This will setup all system-specific
  // resources.  This method and Finalize() must be symmetric and it
  // should be possible to call them multiple times, even changing WindowId
  // in-between.  This is what WindowRemap does.
  virtual void Initialize(void);

  // Description:
  // "Deinitialize" the rendering window.  This will shutdown all system-specific
  // resources.  After having called this, it should be possible to destroy
  // a window that was used for a SetWindowId() call without any ill effects.
  virtual void Finalize(void);

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Resize the window.
  virtual void WindowRemap(void);

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen(void);

  // Description:
  // Specify the size of the rendering window in pixels.
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]) {this->SetSize(a[0], a[1]);}

  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this window the current OpenGL context.
  void MakeCurrent();

  // Description:
  // Tells if this window is the current OpenGL context for the calling thread.
  virtual bool IsCurrent();

  // Description:
  // Does this render window support OpenGL? 0-false, 1-true
  int SupportsOpenGL();

  // Description:
  // Is this render window using hardware acceleration? 0-false, 1-true
  int IsDirect() { return 1;};

  // Description:
  // Get the current size of the screen in pixels.
  virtual int     *GetScreenSize();

  // Description:
  // Get the position in screen coordinates (pixels) of the window.
  virtual int     *GetPosition();

  // Description:
  // Dummy stubs for vtkWindow API.
  virtual void SetDisplayId(void *) {};
  virtual void SetWindowId(void *window) {this->Window = reinterpret_cast<unsigned long>(window);}
  virtual void SetNextWindowId(void *) {}
  virtual void SetParentId(void *)  {}
  virtual void *GetGenericDisplayId();
  virtual void *GetGenericWindowId() {return NULL;}
  virtual void *GetGenericParentId() {return NULL;}
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable() {return NULL;}
  virtual void SetWindowInfo(char *);
  virtual void SetNextWindowInfo(char *) {}
  virtual void SetParentInfo(char *) {}

  void     SetWindowName(const char *);

  // Description:
  // Move the window to a new position on the display.
  void     SetPosition(int,int);
  void     SetPosition(int a[2]) {this->SetPosition(a[0], a[1]);};

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();

  // Description:
  // This computes the size of the render window
  // before calling the supper classes render
  void Render();

  // Description:
  // Check to see if a mouse button has been pressed.  All other events
  // are ignored by this method.  Ideally, you want to abort the render
  // on any event which causes the DesiredUpdateRate to switch from
  // a high-quality rate to a more interactive rate.
  virtual int GetEventPending() { return 0;};

  int GetOwnWindow() { return this->OwnWindow; };
  // Description:
  // Render without displaying the window.
  virtual void SetOffScreenRendering (int value);
  virtual int GetOffScreenRendering ();

  // Description:
  // Returns the width and height of the allocated EGL surface.
  // If no surface is allocated width and height are set to 0.
  void GetEGLSurfaceSize(int* width, int* height);
  // Description:
  // Returns the number of devices (graphics cards) on a system.
  int GetNumberOfDevices();

protected:
  vtkEGLRenderWindow();
  ~vtkEGLRenderWindow();

  unsigned long Window;
  int ScreenSize[2];
  int OwnWindow;
  int DeviceIndex;
  class vtkInternals;
  vtkInternals* Internals;

  void CreateAWindow();
  void DestroyWindow();
  void ResizeWindow(int width, int height);

  // Description:
  // Use EGL_EXT_device_base, EGL_EXT_platform_device and EGL_EXT_platform_base
  // extensions to set the display (output graphics card) to something different than
  // EGL_DEFAULT_DISPLAY. Just use the default display if deviceIndex == 0.
  void SetDeviceAsDisplay(int deviceIndex);

private:
  vtkEGLRenderWindow(const vtkEGLRenderWindow&);  // Not implemented.
  void operator=(const vtkEGLRenderWindow&);  // Not implemented.

  bool DeviceExtensionsPresent;
};



#endif
