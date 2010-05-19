/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMesaRenderWindow - Mesa rendering window
// .SECTION Description
// vtkXMesaRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the OpenGL specific version.

#ifndef __vtkXMesaRenderWindow_h
#define __vtkXMesaRenderWindow_h

#include "vtkMesaRenderWindow.h"
#include <X11/Xlib.h> // Needed for X types used in the public interface
#include <X11/Xutil.h> // Needed for X types used in the public interface

class vtkIdList;
class vtkXMesaRenderWindowInternal;

class VTK_RENDERING_EXPORT vtkXMesaRenderWindow : public vtkMesaRenderWindow
{
public:
  static vtkXMesaRenderWindow *New();
  vtkTypeMacro(vtkXMesaRenderWindow,vtkMesaRenderWindow);
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
  // Initialize the rendering window.
  virtual void Initialize(void);

  // Description:
  // Shut-down all resources initialized by Initialize()
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
  virtual void SetSize(int a[2]) {this->SetSize(a[0], a[1]);};

  // Description:
  // Get the X properties of an ideal rendering window.
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  virtual XVisualInfo     *GetDesiredVisualInfo();
  virtual int      GetDesiredDepth();

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
  // If called, allow MakeCurrent() to skip cache-check when called.
  // MakeCurrent() reverts to original behavior of cache-checking     
  // on the next render.     
  void SetForceMakeCurrent();

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
  // Xwindow get set functions
  virtual void *GetGenericDisplayId() {return (void *)this->GetDisplayId();};
  virtual void *GetGenericWindowId();
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  
  // Description:
  // Get the current size of the screen in pixels.
  virtual int     *GetScreenSize();

  // Description:
  // Get the position in screen coordinates (pixels) of the window.
  virtual int     *GetPosition();

  // Description:
  // Get this RenderWindow's X display id.
  Display *GetDisplayId();

  // Description:
  // Set the X display id for this RenderWindow to use to a pre-existing 
  // X display id.
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);

  // Description:
  // Get this RenderWindow's parent X window id.
  Window   GetParentId();

  // Description:
  // Sets the parent of the window that WILL BE created.
  void     SetParentId(Window);
  void     SetParentId(void *);
  
  // Description:
  // Get this RenderWindow's X window id.
  Window   GetWindowId();

  // Description:
  // Set this RenderWindow's X window id to a pre-existing window.
  void     SetWindowId(Window);
  void     SetWindowId(void *);

  // Description:
  // Specify the X window id to use if a WindowRemap is done.
  void     SetNextWindowId(Window);
  void     SetNextWindowId(void *);
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
  // Change the shape of the cursor
  virtual void SetCurrentCursor(int);

  // Description:
  // Get the properties available at runtime that would result in 
  // the best Xwindow for 3D rendering.
  virtual  int GetEventPending();
  
  // Description:
  // Set this RenderWindow's X window id to a pre-existing window.
  void     SetWindowInfo(char *info);

  void     SetNextWindowInfo(char *);

  // Description:
  // Sets the X window id of the window that WILL BE created.
  void     SetParentInfo(char *info);

  // Description:
  // This computes the size of the render window 
  // before calling the supper classes render
  void Render();  

  // Description:
  // Render without displaying the window.
  void SetOffScreenRendering(int i);

protected:
  vtkXMesaRenderWindow();
  ~vtkXMesaRenderWindow();

  vtkXMesaRenderWindowInternal *Internal;
  
  Window   ParentId;
  Window   WindowId;
  Window   NextWindowId;
  Display *DisplayId;
  Colormap ColorMap;
  int      OwnWindow;
  int      OwnDisplay;
  int      ScreenSize[2];
  int      CursorHidden;
  int      ForceMakeCurrent;
  int      UsingHardware;
  char    *Capabilities;

  // we must keep track of the cursors we are using
  Cursor XCArrow;
  Cursor XCSizeAll;
  Cursor XCSizeNS;
  Cursor XCSizeWE;
  Cursor XCSizeNE;
  Cursor XCSizeNW;
  Cursor XCSizeSE;
  Cursor XCSizeSW;


  void CreateAWindow();
  void DestroyWindow();
  void CreateOffScreenWindow(int x, int y);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int x, int y);
  
private:
  vtkXMesaRenderWindow(const vtkXMesaRenderWindow&);  // Not implemented.
  void operator=(const vtkXMesaRenderWindow&);  // Not implemented.
};



#endif
