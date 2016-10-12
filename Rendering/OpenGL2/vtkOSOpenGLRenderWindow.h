/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSOpenGLRenderWindow
 * @brief   OffScreen Mesa rendering window
 *
 * vtkOSOpenGLRenderWindow is a concrete implementation of the abstract class
 * vtkOpenGLRenderWindow. vtkOSOpenGLRenderWindow interfaces to the OffScreen
 * Mesa software implementation of the OpenGL library. The framebuffer resides
 * on host memory. The framebuffer is the collection of logical buffers
 * (color buffer(s), depth buffer, stencil buffer, accumulation buffer,
 * multisample buffer) defining where the output of GL rendering is directed.
 * Application programmers should normally use vtkRenderWindow instead of the
 * OpenGL specific version.
*/

#ifndef vtkOSOpenGLRenderWindow_h
#define vtkOSOpenGLRenderWindow_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

class vtkIdList;
class vtkOSOpenGLRenderWindowInternal;

class VTKRENDERINGOPENGL2_EXPORT vtkOSOpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkOSOpenGLRenderWindow *New();
  vtkTypeMacro(vtkOSOpenGLRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Begin the rendering process.
   */
  virtual void Start(void);

  /**
   * End the rendering process and display the image.
   */
  virtual void Frame(void);

  /**
   * Initialize the window for rendering.
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
   * "Deinitialize" the rendering window.  This will shutdown all system-specific
   * resources.  After having called this, it should be possible to destroy
   * a window that was used for a SetWindowId() call without any ill effects.
   */
  virtual void Finalize(void);

  /**
   * Change the window to fill the entire screen.
   */
  virtual void SetFullScreen(int);

  //@{
  /**
   * Specify the size of the rendering window in pixels.
   */
  virtual void SetSize(int x,int y);
  virtual void SetSize(int a[2]) {this->SetSize(a[0], a[1]);};
  //@}

  /**
   * Get the current size of the screen in pixels.
   */
  virtual int     *GetScreenSize();

  /**
   * Get the position in screen coordinates (pixels) of the window.
   */
  virtual int     *GetPosition();

  //@{
  /**
   * Move the window to a new position on the display.
   */
  void     SetPosition(int x, int y);
  void     SetPosition(int a[2]) {this->SetPosition(a[0], a[1]);};
  //@}

  /**
   * Prescribe that the window be created in a stereo-capable mode. This
   * method must be called before the window is realized. This method
   * overrides the superclass method since this class can actually check
   * whether the window has been realized yet.
   */
  virtual void SetStereoCapableWindow(int capable);

  /**
   * Make this window the current OpenGL context.
   */
  void MakeCurrent();

  /**
   * Tells if this window is the current OpenGL context for the calling thread.
   */
  virtual bool IsCurrent();

  /**
   * If called, allow MakeCurrent() to skip cache-check when called.
   * MakeCurrent() reverts to original behavior of cache-checking
   * on the next render.
   */
  void SetForceMakeCurrent();

  /**
   * Get report of capabilities for the render window
   */
  const char *ReportCapabilities();

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL();

  /**
   * Is this render window using hardware acceleration? 0-false, 1-true
   */
  int IsDirect();

  /**
   * Resize the window.
   */
  virtual void WindowRemap(void);

  //@{
  /**
   * Xwindow get set functions
   */
  virtual void *GetGenericDisplayId() {return 0;}
  virtual void *GetGenericWindowId();
  virtual void *GetGenericParentId()  {return 0;}
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable()  {return 0;}
  //@}

  /**
   * Set the X display id for this RenderWindow to use to a pre-existing
   * X display id.
   */
  void     SetDisplayId(void *) {}

  /**
   * Sets the parent of the window that WILL BE created.
   */
  void     SetParentId(void *);

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void     SetWindowId(void *);

  /**
   * Set the window id of the new window once a WindowRemap is done.
   * This is the generic prototype as required by the vtkRenderWindow
   * parent.
   */
  void     SetNextWindowId(void *);

  void     SetWindowName(const char *);

  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   */
  void HideCursor() {}
  void ShowCursor() {}

  /**
   * Change the shape of the cursor
   */
  virtual void SetCurrentCursor(int);

  /**
   * Check to see if a mouse button has been pressed.
   * All other events are ignored by this method.
   * This is a useful check to abort a long render.
   */
  virtual  int GetEventPending();

  /**
   * Set this RenderWindow's X window id to a pre-existing window.
   */
  void     SetWindowInfo(char *info);

  /**
   * Set the window info that will be used after WindowRemap()
   */
  void     SetNextWindowInfo(char *info);

  /**
   * Sets the X window id of the window that WILL BE created.
   */
  void     SetParentInfo(char *info);

  /**
   * Render without displaying the window.
   */
  void SetOffScreenRendering(int i);

protected:
  vtkOSOpenGLRenderWindow();
  ~vtkOSOpenGLRenderWindow();

  vtkOSOpenGLRenderWindowInternal *Internal;

  int      OwnWindow;
  int      OwnDisplay;
  int      ScreenSize[2];
  int      CursorHidden;
  int      ForceMakeCurrent;
  char    *Capabilities;

  void CreateAWindow();
  void DestroyWindow();
  void CreateOffScreenWindow(int width, int height);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int width, int height);


private:
  vtkOSOpenGLRenderWindow(const vtkOSOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
};



#endif
