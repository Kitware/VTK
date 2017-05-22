/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXOpenGLRenderWindow
 * @brief   OpenGL rendering window
 *
 * vtkXOpenGLRenderWindow is a concrete implementation of the abstract class
 * vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
 * library. Application programmers should normally use vtkRenderWindow
 * instead of the OpenGL specific version.
*/

#ifndef vtkXOpenGLRenderWindow_h
#define vtkXOpenGLRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include <stack> // for ivar
#include "vtkOpenGLRenderWindow.h"
#include <X11/Xlib.h> // Needed for X types used in the public interface
#include <X11/Xutil.h> // Needed for X types used in the public interface

class vtkIdList;
class vtkXOpenGLRenderWindowInternal;

class VTKRENDERINGOPENGL2_EXPORT vtkXOpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkXOpenGLRenderWindow *New();
  vtkTypeMacro(vtkXOpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Begin the rendering process.
   */
  void Start() VTK_OVERRIDE;

  /**
   * End the rendering process and display the image.
   */
  void Frame() VTK_OVERRIDE;

  /**
   * Initialize the window for rendering.
   */
  virtual void WindowInitialize();

  /**
   * Initialize the rendering window.  This will setup all system-specific
   * resources.  This method and Finalize() must be symmetric and it
   * should be possible to call them multiple times, even changing WindowId
   * in-between.  This is what WindowRemap does.
   */
  void Initialize() VTK_OVERRIDE;

  /**
   * "Deinitialize" the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  void Finalize() VTK_OVERRIDE;

  /**
   * Change the window to fill the entire screen.
   */
  virtual void SetFullScreen(int) VTK_OVERRIDE;

  /**
   * Resize the window.
   */
  void WindowRemap() VTK_OVERRIDE;

  /**
   * Set the preferred window size to full screen.
   */
  virtual void PrefFullScreen();

  /**
   * Specify the size of the rendering window in pixels.
   */
  void SetSize(int,int) VTK_OVERRIDE;
  void SetSize(int a[2]) VTK_OVERRIDE { this->SetSize(a[0], a[1]); }

  //@{
  /**
   * Get the X properties of an ideal rendering window.
   */
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  virtual XVisualInfo     *GetDesiredVisualInfo();
  virtual int GetDesiredDepth();
  //@}

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  void SetStereoCapableWindow(int capable) VTK_OVERRIDE;

  /**
   * Make this window the current OpenGL context.
   */
  void MakeCurrent() VTK_OVERRIDE;

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  bool IsCurrent() VTK_OVERRIDE;

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  void SetForceMakeCurrent() VTK_OVERRIDE;

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities() VTK_OVERRIDE;

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect() VTK_OVERRIDE;

  /**
   * Xwindow get set functions
   */
  void *GetGenericDisplayId() VTK_OVERRIDE
  {
      return this->GetDisplayId();
  }

  void *GetGenericWindowId() VTK_OVERRIDE;
  void *GetGenericParentId() VTK_OVERRIDE
  {
      return reinterpret_cast<void *>(this->ParentId);
  }

  void *GetGenericContext() VTK_OVERRIDE;
  void *GetGenericDrawable() VTK_OVERRIDE
  {
      return reinterpret_cast<void *>(this->WindowId);
  }

  /**
   * Get the current size of the screen in pixels.
   */
  int *GetScreenSize() VTK_OVERRIDE;

  /**
   * Get the position in screen coordinates (pixels) of the window.
   */
  int *GetPosition() VTK_OVERRIDE;

  /**
   * Get this RenderWindow's X display id.
   */
  Display *GetDisplayId();

  //@{
  /**
   * Set the X display id for this RenderWindow to use to a pre-existing
   * X display id.
   */
  void SetDisplayId(Display *);
  void SetDisplayId(void *) VTK_OVERRIDE;
  //@}

  /**
   * Get this RenderWindow's parent X window id.
   */
  Window GetParentId();

  //@{
  /**
   * Sets the parent of the window that WILL BE created.
   */
  void SetParentId(Window);
  void SetParentId(void *) VTK_OVERRIDE;
  //@}

  /**
   * Get this RenderWindow's X window id.
   */
  Window GetWindowId();

  //@{
  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowId(Window);
  void SetWindowId(void *) VTK_OVERRIDE;
  //@}

  /**
   * Specify the X window id to use if a WindowRemap is done.
   */
  void SetNextWindowId(Window);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void SetNextWindowId(void *) VTK_OVERRIDE;

  /**
   * Set name of rendering window.
   */
  void SetWindowName(const char *) VTK_OVERRIDE;

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() VTK_OVERRIDE;

  //@{
  /**
   * Move the window to a new position on the display.
   */
  void SetPosition(int,int) VTK_OVERRIDE;
  void SetPosition(int a[2]) VTK_OVERRIDE { this->SetPosition(a[0], a[1]); }
  //@}

  //@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() VTK_OVERRIDE;
  void ShowCursor() VTK_OVERRIDE;
  //@}

  /**
   * Change the shape of the cursor
   */
  void SetCurrentCursor(int) VTK_OVERRIDE;

  /**
   * Check to see if a mouse button has been pressed or mouse wheel activated.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  int GetEventPending() VTK_OVERRIDE;

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void SetWindowInfo(char *info) VTK_OVERRIDE;

  /**
   * Set the window info that will be used after WindowRemap()
   */
  void SetNextWindowInfo(char *info) VTK_OVERRIDE;

  /**
   * Sets the X window id of the window that WILL BE created.
   */
  void SetParentInfo(char *info) VTK_OVERRIDE;

  /**
   * This computes the size of the render window
   * before calling the supper classes render
   */
  void Render() VTK_OVERRIDE;

  /**
   * Render without displaying the window.
   */
  void SetOffScreenRendering(int i) VTK_OVERRIDE;

  //@{
  /**
   * Ability to push and pop this window's context
   * as the current context. The idea being to
   * if needed make this window's context current
   * and when done releasing resources restore
   * the prior context
   */
  void PushContext() VTK_OVERRIDE;
  void PopContext() VTK_OVERRIDE;
  //@}

protected:
  vtkXOpenGLRenderWindow();
  ~vtkXOpenGLRenderWindow() VTK_OVERRIDE;

  vtkXOpenGLRenderWindowInternal *Internal;

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

  std::stack<Display *> DisplayStack;
  std::stack<Drawable> DrawableStack;
  std::stack<void *> ContextStack;

  // we must keep track of the cursors we are using
  Cursor XCCrosshair;
  Cursor XCArrow;
  Cursor XCSizeAll;
  Cursor XCSizeNS;
  Cursor XCSizeWE;
  Cursor XCSizeNE;
  Cursor XCSizeNW;
  Cursor XCSizeSE;
  Cursor XCSizeSW;
  Cursor XCHand;


  void CreateAWindow() VTK_OVERRIDE;
  void DestroyWindow() VTK_OVERRIDE;
  void CreateOffScreenWindow(int width, int height);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int width, int height);
  void CloseDisplay();


private:
  vtkXOpenGLRenderWindow(const vtkXOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
};



#endif
