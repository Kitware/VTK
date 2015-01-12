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
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the OpenGL specific version.

#ifndef vtkEGLRenderWindow_h
#define vtkEGLRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

#include <EGL/egl.h> // required for EGL members

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
  virtual void SetWindowId(void *window)  { this->Window = (ANativeWindow *)window;};
  virtual void SetNextWindowId(void *) {};
  virtual void SetParentId(void *)  {};
  virtual void *GetGenericDisplayId() { return this->Display; };
  virtual void *GetGenericWindowId() {};
  virtual void *GetGenericParentId() {};
  virtual void *GetGenericContext() { return this->Context; };
  virtual void *GetGenericDrawable() {};
  virtual void SetWindowInfo(char *);
  virtual void SetNextWindowInfo(char *) {};
  virtual void SetParentInfo(char *) {};

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
  // Render without displaying the window.
  void SetOffScreenRendering(int i);

  // Description:
  // Check to see if a mouse button has been pressed.  All other events
  // are ignored by this method.  Ideally, you want to abort the render
  // on any event which causes the DesiredUpdateRate to switch from
  // a high-quality rate to a more interactive rate.
  virtual int GetEventPending() { return 0;};

  int GetOwnWindow() { return this->OwnWindow; };

protected:
  vtkEGLRenderWindow();
  ~vtkEGLRenderWindow();

  ANativeWindow *Window;
  EGLDisplay Display;
  EGLSurface Surface;
  EGLContext Context;
  int ScreenSize[2];
  int OwnWindow;

  void CreateAWindow();
  void DestroyWindow();
  void CreateOffScreenWindow(int width, int height);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int width, int height);

private:
  vtkEGLRenderWindow(const vtkEGLRenderWindow&);  // Not implemented.
  void operator=(const vtkEGLRenderWindow&);  // Not implemented.
};



#endif
