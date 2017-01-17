/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericOpenGLRenderWindow
 * @brief   platform independent render window
 *
 *
 * vtkGenericOpenGLRenderWindow provides a skeleton for implementing a render window
 * using one's own OpenGL context and drawable.
 * To be effective, one must register an observer for WindowMakeCurrentEvent,
 * WindowIsCurrentEvent and WindowFrameEvent.  When this class sends a WindowIsCurrentEvent,
 * the call data is an bool* which one can use to return whether the context is current.
*/

#ifndef vtkGenericOpenGLRenderWindow_h
#define vtkGenericOpenGLRenderWindow_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGLRenderWindow.h"

class VTKRENDERINGOPENGL_EXPORT vtkGenericOpenGLRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkGenericOpenGLRenderWindow* New();
  vtkTypeMacro(vtkGenericOpenGLRenderWindow, vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
protected:
  vtkGenericOpenGLRenderWindow();
  ~vtkGenericOpenGLRenderWindow() VTK_OVERRIDE;

public:

  //! Cleans up graphics resources allocated in the context for this VTK scene.
  void Finalize() VTK_OVERRIDE;

  //! flush the pending drawing operations
  //! Class user may to watch for WindowFrameEvent and act on it
  void Frame() VTK_OVERRIDE;

  //! Makes the context current.  It is the class user's
  //! responsibility to watch for WindowMakeCurrentEvent and set it current.
  void MakeCurrent() VTK_OVERRIDE;

  //! Returns if the context is current.  It is the class user's
  //! responsibility to watch for WindowIsCurrentEvent and set the bool* flag
  //! passed through the call data parameter.
  bool IsCurrent() VTK_OVERRIDE;

  //! Returns if OpenGL is supported.  It is the class user's
  //! responsibility to watch for WindowSupportsOpenGLEvent and set the int* flag
  //! passed through the call data parameter.
  int SupportsOpenGL() VTK_OVERRIDE;

  //! Returns if the context is direct.  It is the class user's
  //! responsibility to watch for WindowIsDirectEvent and set the int* flag
  //! passed through the call data parameter.
  int IsDirect() VTK_OVERRIDE;

  // {@
  //! set the drawing buffers to use
  void SetFrontBuffer(unsigned int);
  void SetFrontLeftBuffer(unsigned int);
  void SetFrontRightBuffer(unsigned int);
  void SetBackBuffer(unsigned int);
  void SetBackLeftBuffer(unsigned int);
  void SetBackRightBuffer(unsigned int);
  // }@

  //! convenience function to push the state and push/init the transform matrices
  void PushState();
  //! convenience function to pop the state and pop the transform matrices
  void PopState();

  // {@
  //! does nothing
  void SetWindowId(void*) VTK_OVERRIDE;
  void* GetGenericWindowId() VTK_OVERRIDE;
  void SetDisplayId(void*) VTK_OVERRIDE;
  void SetParentId(void*) VTK_OVERRIDE;
  void* GetGenericDisplayId() VTK_OVERRIDE;
  void* GetGenericParentId() VTK_OVERRIDE;
  void* GetGenericContext() VTK_OVERRIDE;
  void* GetGenericDrawable() VTK_OVERRIDE;
  void SetWindowInfo(char*) VTK_OVERRIDE;
  void SetParentInfo(char*) VTK_OVERRIDE;
  int* GetScreenSize() VTK_OVERRIDE;
  void Start() VTK_OVERRIDE;
  void HideCursor() VTK_OVERRIDE;
  void ShowCursor() VTK_OVERRIDE;
  void SetFullScreen(int) VTK_OVERRIDE;
  void WindowRemap() VTK_OVERRIDE;
  int  GetEventPending() VTK_OVERRIDE;
  void SetNextWindowId(void*) VTK_OVERRIDE;
  void SetNextWindowInfo(char*) VTK_OVERRIDE;
  void CreateAWindow() VTK_OVERRIDE;
  void DestroyWindow() VTK_OVERRIDE;
  // }@

  //@{
  /**
   * Allow to update state within observer callback without changing
   * data argument and MTime.
   */
  void SetIsDirect(int newValue);
  void SetSupportsOpenGL(int newValue);
  void SetIsCurrent(bool newValue);
  //@}

protected:
  int DirectStatus;
  int SupportsOpenGLStatus;
  bool CurrentStatus;

private:
  vtkGenericOpenGLRenderWindow(const vtkGenericOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericOpenGLRenderWindow&) VTK_DELETE_FUNCTION;
};

#endif
