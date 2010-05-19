/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWinCEOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWinCEOpenGLRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkWinCEOpenGLRenderWindow is a concrete implementation of the abstract
// class vtkRenderWindow. vtkWinCEOpenGLRenderer interfaces to the standard
// OpenGL graphics library in the Windows/NT environment..

#ifndef __vtkWinCEOpenGLRenderWindow_h
#define __vtkWinCEOpenGLRenderWindow_h

#include "vtkOpenGLRenderWindow.h"

#include "GL/osmesa.h" // Needed for Mesa types

class vtkIdList;

class VTK_RENDERING_EXPORT vtkWinCEOpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkWinCEOpenGLRenderWindow *New();
  vtkTypeMacro(vtkWinCEOpenGLRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // End the rendering process and display the image.
  void Frame(void);

  // Description:
  // Create the window
  virtual void WindowInitialize(void);

  // Description:
  // Initialize the rendering window.  This will setup all system-specific
  // resources.  This method and Finalize() must be symmetric and it
  // should be possible to call them multiple times, even changing WindowId
  // in-between.  This is what WindowRemap does.
  virtual void Initialize(void);

  // Description:
  // Finalize the rendering window.  This will shutdown all system-specific
  // resources.  After having called this, it should be possible to destroy
  // a window that was used for a SetWindowId() call without any ill effects.
  virtual void Finalize(void);

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Remap the window.
  virtual void WindowRemap(void);

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen(void);

  // Description:
  // Set the size of the window in pixels.
  virtual void SetSize(int,int);

  // Description:
  // Get the current size of the window in pixels.
  virtual int *GetSize();

  // Description:
  // Set the position of the window.
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
  virtual void SetWindowName(char *);
  
  // Description:
  // Set this RenderWindow's window id to a pre-existing window.
  void SetWindowInfo(char *);

  void SetNextWindowInfo(char *);

  // Description:
  // Sets the HWND id of the window that WILL BE created.
  void SetParentInfo(char *);

  //BTX
  virtual void *GetGenericDisplayId() {return (void *)this->OffScreenContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void SetDisplayId(void *) {};

  // Description:
  // Get the window id.
  virtual HWND  GetWindowId();
  void  SetWindowId(void *foo) {this->SetWindowId((HWND)foo);};

  // Description:
  // Set the window id to a pre-existing window.
  virtual void  SetWindowId(HWND);
  
  // Description:
  // Set the window's parent id to a pre-existing window.
  virtual void  SetParentId(HWND);
  void  SetParentId(void *foo) {this->SetParentId((HWND)foo);};

  // Description:
  // Set the window id of the new window once a WindowRemap is done.
  virtual void  SetNextWindowId(HWND);

  virtual void SetNextWindowId(void *);
  //ETX

  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // If called, allow MakeCurrent() to skip cache-check when called.
  // MakeCurrent() reverts to original behavior of cache-checking
  // on the next render.
  void SetForceMakeCurrent();

  // Description:
  // Check to see if an event is pending for this window.
  // This is a useful check to abort a long render.
  virtual  int GetEventPending();

  // Description:
  // Clean up device contexts, rendering contexts, etc.
  void Clean();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();

protected:
  vtkWinCEOpenGLRenderWindow();
  ~vtkWinCEOpenGLRenderWindow();

  HINSTANCE ApplicationInstance;

  OSMesaContext OffScreenContextId;
  void *OffScreenWindow;

  HDC       DeviceContext;
  HWND      WindowId;
  HWND      ParentId;
  HWND      NextWindowId;
  int       OwnWindow;
  int       ScreenSize[2];

  //BTX
  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message, 
                              WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, 
                                  WPARAM wParam, LPARAM lParam);
  //ETX
  int CursorHidden;
  int ForceMakeCurrent;

  void ResizeWhileOffscreen(int xsize, int ysize);
  void CreateAWindow(int x, int y, int width, int height);
  void InitializeApplication();
private:
  vtkWinCEOpenGLRenderWindow(const vtkWinCEOpenGLRenderWindow&);  // Not implemented.
  void operator=(const vtkWinCEOpenGLRenderWindow&);  // Not implemented.
};


#endif

