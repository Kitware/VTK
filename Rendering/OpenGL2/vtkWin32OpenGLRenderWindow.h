/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkWin32OpenGL2RenderWindow.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWin32OpenGL2RenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkWin32OpenGL2RenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkWin32OpenGL2Renderer interfaces to the standard
 * OpenGL graphics library in the Windows/NT environment..
*/

#ifndef vtkWin32OpenGLRenderWindow_h
#define vtkWin32OpenGLRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <stack> // for ivar
#include "vtkOpenGLRenderWindow.h"

#include "vtkWindows.h" // For windows API

class vtkIdList;

class VTKRENDERINGOPENGL2_EXPORT vtkWin32OpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkWin32OpenGLRenderWindow *New();
  vtkTypeMacro(vtkWin32OpenGLRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Begin the rendering process.
   */
  virtual void Start(void);

  /**
   * End the rendering process and display the image.
   */
  void Frame(void);

  /**
   * Create the window
   */
  virtual void WindowInitialize(void);

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  virtual void Initialize(void);

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  virtual void Finalize(void);

  /**
   * Change the window to fill the entire screen.
   */
  virtual void SetFullScreen(int);

  /**
   * Remap the window.
   */
  virtual void WindowRemap(void);

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen(void);

  //@{
  /**
   * Set the size of the window in pixels.
   */
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]) {vtkOpenGLRenderWindow::SetSize(a);};
  //@}

  /**
   * Get the current size of the window in pixels.
   */
  virtual int *GetSize();

  //@{
  /**
   * Set the position of the window.
   */
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]) {vtkOpenGLRenderWindow::SetPosition(a);};
  //@}

  /**
   * Get the current size of the screen in pixels.
   */
  virtual int *GetScreenSize();

  /**
   * Get the position in screen coordinates of the window.
   */
  virtual int *GetPosition();

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  virtual void SetWindowName(const char *);

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   */
  void SetWindowInfo(char *);

  /**
   * Sets the WindowInfo that will be used after a WindowRemap.
   */
  void SetNextWindowInfo(char *);

  /**
   * Sets the HWND id of the window that WILL BE created.
   */
  void SetParentInfo(char *);

  virtual void *GetGenericDisplayId() {return (void *)this->ContextId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext()   {return (void *)this->DeviceContext;};
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  virtual void SetDisplayId(void *);

  /**
   * Get the window id.
   */
  virtual HWND  GetWindowId();

  //@{
  /**
   * Set the window id to a pre-existing window.
   */
  virtual void  SetWindowId(HWND);
  void  SetWindowId(void *foo) {this->SetWindowId((HWND)foo);};
  //@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  virtual bool InitializeFromCurrentContext();

  //@{
  /**
   * Set the window's parent id to a pre-existing window.
   */
  virtual void  SetParentId(HWND);
  void  SetParentId(void *foo) {this->SetParentId((HWND)foo);};
  //@}

  void  SetContextId(HGLRC);    // hsr
  void  SetDeviceContext(HDC);  // hsr

  /**
   * Set the window id of the new window once a WindowRemap is done.
   */
  virtual void  SetNextWindowId(HWND);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  virtual void SetNextWindowId(void *arg);

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  virtual void SetStereoCapableWindow(int capable);

  /**
   * Make this windows OpenGL context the current context.
   */
  void MakeCurrent();

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  virtual bool IsCurrent();

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities();

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect();

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  virtual  int GetEventPending();

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(HDC hDC);
  virtual void SetupPixelFormatPaletteAndContext(
    HDC hDC, DWORD dwFlags, int debug,
    int bpp=16, int zbpp=16);
  //@}

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor();
  void ShowCursor();
  void SetCursorPosition(int x, int y);
  //@}

  /**
   * Change the shape of the cursor
   */
  virtual void SetCurrentCursor(int);

  virtual bool DetectDPI();

  /**
   * Override the default implementation so that we can actively switch between
   * on and off screen rendering.
   */
  virtual void SetOffScreenRendering(int offscreen);

  //@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  virtual void PushContext();
  virtual void PopContext();
  //@}

protected:
  vtkWin32OpenGLRenderWindow();
  ~vtkWin32OpenGLRenderWindow();

  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HPALETTE  OldPalette;
  HGLRC     ContextId;
  HDC       DeviceContext;
  BOOL      MFChandledWindow;
  HWND      WindowId;
  HWND      ParentId;
  HWND      NextWindowId;
  int       OwnWindow;
  int       ScreenSize[2];

  std::stack<HGLRC> ContextStack;
  std::stack<HDC> DCStack;

  int CreatingOffScreenWindow; // to avoid recursion (and memory leaks...)

  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message,
                              WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message,
                                  WPARAM wParam, LPARAM lParam);
  int CursorHidden;
  int ForceMakeCurrent;

  char   *Capabilities;
  int WindowIdReferenceCount;
  void ResizeWhileOffscreen(int xsize, int ysize);
  virtual void CreateAWindow();
  virtual void DestroyWindow();
  void InitializeApplication();
  void CleanUpOffScreenRendering(void);
  void CreateOffScreenWindow(int width,int height);
  void CleanUpRenderers();
  void VTKRegisterClass();

private:
  vtkWin32OpenGLRenderWindow(const vtkWin32OpenGLRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWin32OpenGLRenderWindow&) VTK_DELETE_FUNCTION;
};


#endif
