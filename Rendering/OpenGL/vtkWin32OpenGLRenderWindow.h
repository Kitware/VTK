/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWin32OpenGLRenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkWin32OpenGLRenderWindow is a concrete implementation of the abstract
 * class vtkRenderWindow. vtkWin32OpenGLRenderer interfaces to the standard
 * OpenGL graphics library in the Windows environment.
*/

#ifndef vtkWin32OpenGLRenderWindow_h
#define vtkWin32OpenGLRenderWindow_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

class vtkIdList;

class VTKRENDERINGOPENGL_EXPORT vtkWin32OpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkWin32OpenGLRenderWindow *New();
  vtkTypeMacro(vtkWin32OpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Begin the rendering process.
   */
  void Start(void) VTK_OVERRIDE;

  /**
   * End the rendering process and display the image.
   */
  void Frame(void) VTK_OVERRIDE;

  /**
   * Create the window
   */
  virtual void WindowInitialize(void);

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between. This is what WindowRemap does.
   */
  void Initialize(void) VTK_OVERRIDE;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources. After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize(void) VTK_OVERRIDE;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(int) VTK_OVERRIDE;

  /**
   * Remap the window.
   */
  void WindowRemap(void) VTK_OVERRIDE;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen(void);

  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int width, int height) VTK_OVERRIDE;
  void SetSize(int a[2]) VTK_OVERRIDE { vtkOpenGLRenderWindow::SetSize(a); }

  /**
   * Get the current size of the window in pixels.
   */
  int *GetSize() VTK_OVERRIDE;

  /**
   * Set the position of the window.
   */
  void SetPosition(int x, int y) VTK_OVERRIDE;
  void SetPosition(int a[2]) VTK_OVERRIDE { vtkOpenGLRenderWindow::SetPosition(a); }

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
  void SetWindowName(const char*) VTK_OVERRIDE;

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   */
  void SetWindowInfo(char*) VTK_OVERRIDE;

  /**
   * Sets the WindowInfo that will be used after a WindowRemap.
   */
  void SetNextWindowInfo(char*) VTK_OVERRIDE;

  /**
   * Sets the HWND id of the window that WILL BE created.
   */
  void SetParentInfo(char*);

  void *GetGenericDisplayId() VTK_OVERRIDE { return (void*)this->ContextId; }
  void *GetGenericWindowId() VTK_OVERRIDE { return (void*)this->WindowId; }
  void *GetGenericParentId() VTK_OVERRIDE { return (void*)this->ParentId; }
  void *GetGenericContext() VTK_OVERRIDE { return (void*)this->DeviceContext; }
  void *GetGenericDrawable() VTK_OVERRIDE { return (void*)this->WindowId; }
  void SetDisplayId(void*) VTK_OVERRIDE;

  /**
   * Get the window id.
   */
  virtual HWND GetWindowId();

  /**
   * Set the window id to a pre-existing window.
   */
  virtual void SetWindowId(HWND);
  void SetWindowId(void *foo) VTK_OVERRIDE { this->SetWindowId((HWND)foo); }

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() VTK_OVERRIDE;

  /**
   * Set the window's parent id to a pre-existing window.
   */
  virtual void SetParentId(HWND);
  void SetParentId(void *foo) VTK_OVERRIDE { this->SetParentId((HWND)foo); }

  /**
   * Set the window's context id
   */
  void SetContextId(HGLRC);

  /**
   * Set the window's device context
   */
  void SetDeviceContext(HDC);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   */
  virtual void SetNextWindowId(HWND);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void *arg) VTK_OVERRIDE;

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
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
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities() VTK_OVERRIDE;

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL();

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() VTK_OVERRIDE;

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  int GetEventPending() VTK_OVERRIDE;

  //@{
  /**
   * These methods can be used by MFC applications
   * to support print preview and printing, or more
   * general rendering into memory.
   */
  void SetupMemoryRendering(int x, int y, HDC prn);
  void SetupMemoryRendering(HBITMAP hbmp);
  void ResumeScreenRendering(void);
  HDC GetMemoryDC();
  unsigned char *GetMemoryData() { return this->MemoryData; }
  //@}

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(HDC hDC);
  virtual void SetupPixelFormat(HDC hDC, DWORD dwFlags, int debug,
                                int bpp = 16, int zbpp = 16);
  //@}

  /**
   * Clean up device contexts, rendering contexts, etc.
   */
  void Clean();

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() VTK_OVERRIDE;
  void ShowCursor() VTK_OVERRIDE;
  //@}
  /**
   * Set cursor position in window (note that (0,0) is the lower left corner).
   */
  void SetCursorPosition(int x, int y) VTK_OVERRIDE;

  /**
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) VTK_OVERRIDE;

  bool DetectDPI() VTK_OVERRIDE;

  /**
   * Override the default implementation so that we can actively switch between
   * on and off screen rendering.
   */
  void SetOffScreenRendering(int offscreen) VTK_OVERRIDE;

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

  // the following is used to support rendering into memory
  BITMAPINFO MemoryDataHeader;
  HBITMAP MemoryBuffer;
  unsigned char *MemoryData;    // the data in the DIBSection
  HDC MemoryHdc;

  int ScreenMapped;
  int ScreenWindowSize[2];
  HDC ScreenDeviceContext;
  int ScreenDoubleBuffer;
  HGLRC ScreenContextId;

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
  void CreateAWindow() VTK_OVERRIDE;
  void DestroyWindow() VTK_OVERRIDE;
  void InitializeApplication();
  void CleanUpOffScreenRendering(void);
  void CreateOffScreenDC(int xsize, int ysize, HDC aHdc);
  void CreateOffScreenDC(HBITMAP hbmp, HDC aHdc);
  void CreateOffScreenWindow(int width, int height);
  void SaveScreenRendering();
  void CleanUpRenderers();

private:
  vtkWin32OpenGLRenderWindow(const vtkWin32OpenGLRenderWindow&) = delete;
  void operator=(const vtkWin32OpenGLRenderWindow&) = delete;
};

#endif
