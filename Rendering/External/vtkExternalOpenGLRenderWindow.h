/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExternalOpenGLRenderWindow - OpenGL render window that allows using
// an external window to render vtk objects
// .SECTION Description
// vtkExternalOpenGLRenderWindow is a concrete implementation of the abstract
// class vtkRenderWindow. vtkExternalOpenGLRenderer interfaces to the OpenGL
// graphics library.
//
// This class extends vtkGenericOpenGLRenderWindow to allow sharing the
// same OpenGL context by various visualization applications. Basically, this
// class prevents VTK from creating a new OpenGL context. Thus, it requires that
// an OpenGL context be initialized before Render is called.
// \sa Render()
//
// It is a generic implementation; this window is platform agnostic. However,
// the application user must explicitly make sure the window size is
// synchronized when the external application window/viewport resizes.
// \sa SetSize()
//
// It has the same requirements as the vtkGenericOpenGLRenderWindow, whereby,
// one must register an observer for WindowMakeCurrentEvent,
// WindowIsCurrentEvent and WindowFrameEvent.
// \sa vtkGenericOpenGLRenderWindow

#ifndef __vtkExternalOpenGLRenderWindow_h
#define __vtkExternalOpenGLRenderWindow_h

#include "vtkRenderingExternalModule.h" // For export macro
#include "vtkGenericOpenGLRenderWindow.h"

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalOpenGLRenderWindow :
  public vtkGenericOpenGLRenderWindow
{
public:
  static vtkExternalOpenGLRenderWindow *New();
  vtkTypeMacro(vtkExternalOpenGLRenderWindow, vtkGenericOpenGLRenderWindow);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Begin the rendering process using the existing context.
  void Start(void);

  // Description:
  // This computes the size of the render window
  // before calling the superclass' Render() method
  void Render();

protected:
  vtkExternalOpenGLRenderWindow();
  ~vtkExternalOpenGLRenderWindow();

private:
  vtkExternalOpenGLRenderWindow(const vtkExternalOpenGLRenderWindow&); // Not implemented
  void operator=(const vtkExternalOpenGLRenderWindow&); // Not implemented
};
#endif //__vtkExternalOpenGLRenderWindow_h
