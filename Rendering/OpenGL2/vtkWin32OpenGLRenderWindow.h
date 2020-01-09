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

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <stack>                       // for ivar

#include "vtkWindows.h" // For windows API

class vtkIdList;

class VTKRENDERINGOPENGL2_EXPORT vtkWin32OpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkWin32OpenGLRenderWindow* New();
  vtkTypeMacro(vtkWin32OpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * End the rendering process and display the image.
   */
  void Frame(void) override;

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
  void Initialize(void) override;

  /**
   * Finalize the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize(void) override;

  /**
   * Change the window to fill the entire screen.
   */
  void SetFullScreen(vtkTypeBool) override;

  /**
   * Remap the window.
   */
  void WindowRemap(void) override;

  /**
   * Show or not Show the window
   */
  void SetShowWindow(bool val) override;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen(void);

  //@{
  /**
   * Set the size of the window in pixels.
   */
  void SetSize(int, int) override;
  void SetSize(int a[2]) override { vtkOpenGLRenderWindow::SetSize(a); }
  //@}

  /**
   * Get the current size of the window in pixels.
   */
  int* GetSize() VTK_SIZEHINT(2) override;

  //@{
  /**
   * Set the position of the window.
   */
  void SetPosition(int, int) override;
  void SetPosition(int a[2]) override { vtkOpenGLRenderWindow::SetPosition(a); }
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

  /**
   * Set this RenderWindow's window id to a pre-existing window.
   */
  void SetWindowInfo(const char*) override;

  /**
   * Sets the WindowInfo that will be used after a WindowRemap.
   */
  void SetNextWindowInfo(const char*) override;

  /**
   * Sets the HWND id of the window that WILL BE created.
   */
  void SetParentInfo(const char*) override;

  void* GetGenericDisplayId() override { return (void*)this->ContextId; }
  void* GetGenericWindowId() override { return (void*)this->WindowId; }
  void* GetGenericParentId() override { return (void*)this->ParentId; }
  void* GetGenericContext() override { return (void*)this->DeviceContext; }
  void* GetGenericDrawable() override { return (void*)this->WindowId; }
  void SetDisplayId(void*) override;

  /**
   * Get the window id.
   */
  HWND GetWindowId();

  //@{
  /**
   * Set the window id to a pre-existing window.
   */
  void SetWindowId(HWND);
  void SetWindowId(void* foo) override { this->SetWindowId((HWND)foo); }
  //@}

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Does this platform support render window data sharing.
   */
  bool GetPlatformSupportsRenderWindowSharing() override { return true; }

  //@{
  /**
   * Set the window's parent id to a pre-existing window.
   */
  void SetParentId(HWND);
  void SetParentId(void* foo) override { this->SetParentId((HWND)foo); }
  //@}

  void SetContextId(HGLRC);   // hsr
  void SetDeviceContext(HDC); // hsr

  /**
   * Set the window id of the new window once a WindowRemap is done.
   */
  void SetNextWindowId(HWND);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void* arg) override;

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
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() override;

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  int GetEventPending() override;

  //@{
  /**
   * Initialize OpenGL for this window.
   */
  virtual void SetupPalette(HDC hDC);
  virtual void SetupPixelFormatPaletteAndContext(
    HDC hDC, DWORD dwFlags, int debug, int bpp = 16, int zbpp = 16);
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
  void HideCursor() override;
  void ShowCursor() override;
  void SetCursorPosition(int x, int y) override;
  //@}

  /**
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) override;

  bool DetectDPI() override;

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

  /**
   * Set the number of vertical syncs required between frames.
   * A value of 0 means swap buffers as quickly as possible
   * regardless of the vertical refresh. A value of 1 means swap
   * buffers in sync with the vertical refresh to eliminate tearing.
   * A value of -1 means use a value of 1 unless we missed a frame
   * in which case swap immediately. Returns true if the call
   * succeeded.
   */
  bool SetSwapControl(int i) override;

protected:
  vtkWin32OpenGLRenderWindow();
  ~vtkWin32OpenGLRenderWindow() override;

  HINSTANCE ApplicationInstance;
  HPALETTE Palette;
  HPALETTE OldPalette;
  HGLRC ContextId;
  HDC DeviceContext;
  BOOL MFChandledWindow;
  HWND WindowId;
  HWND ParentId;
  HWND NextWindowId;
  int OwnWindow;
  int ScreenSize[2];

  std::stack<HGLRC> ContextStack;
  std::stack<HDC> DCStack;

  int CreatingOffScreenWindow; // to avoid recursion (and memory leaks...)

  // message handler
  virtual LRESULT MessageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

  static LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  int CursorHidden;
  int ForceMakeCurrent;

  int WindowIdReferenceCount;
  void ResizeWhileOffscreen(int xsize, int ysize);
  void CreateAWindow() override;
  void DestroyWindow() override;
  void InitializeApplication();
  void CleanUpRenderers();
  void VTKRegisterClass();

private:
  vtkWin32OpenGLRenderWindow(const vtkWin32OpenGLRenderWindow&) = delete;
  void operator=(const vtkWin32OpenGLRenderWindow&) = delete;
};

#endif
