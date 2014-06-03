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
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library.

#ifndef __vtkExternalOpenGLRenderWindow_h
#define __vtkExternalOpenGLRenderWindow_h

#include "vtkRenderingExternalModule.h" // For export macro
#include "vtkXOpenGLRenderWindow.h"

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalOpenGLRenderWindow :
  public vtkXOpenGLRenderWindow
{
public:
  static vtkExternalOpenGLRenderWindow *New();
  vtkTypeMacro(vtkExternalOpenGLRenderWindow, vtkXOpenGLRenderWindow);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Begin the rendering process by either drawing a window or using the
  // existing context. The decision is made based on the DrawWindow flag.
  void Start(void);

  // Description:
  // Set/Get whether to draw own window or use current context.
  // This class performs the same function as a vtkXOpenGLRenderWindow when
  // DrawWindow is set to true.
  // By default, DrawWindow is false.
  vtkSetMacro(DrawWindow, int);
  vtkGetMacro(DrawWindow, int);
  vtkBooleanMacro(DrawWindow, int);

  // Description:
  // This computes the size of the render window
  // before calling the superclass' Render() method
  void Render();

protected:
  vtkExternalOpenGLRenderWindow();
  ~vtkExternalOpenGLRenderWindow();

  int DrawWindow;
private:
  vtkExternalOpenGLRenderWindow(const vtkExternalOpenGLRenderWindow&); // Not implemented
  void operator=(const vtkExternalOpenGLRenderWindow&); // Not implemented
};
#endif //__vtkExternalOpenGLRenderWindow_h
