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
// vtkExternalOpenGLRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkExternalOpenGLRenderer interfaces to the OpenGL graphics
// library.

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

  //bool Initialized;

private:
  vtkExternalOpenGLRenderWindow(const vtkExternalOpenGLRenderWindow&); // Not implemented
  void operator=(const vtkExternalOpenGLRenderWindow&); // Not implemented
};
#endif //__vtkExternalOpenGLRenderWindow_h
