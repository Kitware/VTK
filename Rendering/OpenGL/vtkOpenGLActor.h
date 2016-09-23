/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLActor
 * @brief   OpenGL actor
 *
 * vtkOpenGLActor is a concrete implementation of the abstract class vtkActor.
 * vtkOpenGLActor interfaces to the OpenGL rendering library.
*/

#ifndef vtkOpenGLActor_h
#define vtkOpenGLActor_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkActor.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLActor : public vtkActor
{
public:
  static vtkOpenGLActor *New();
  vtkTypeMacro(vtkOpenGLActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer *ren, vtkMapper *mapper);

protected:
  vtkOpenGLActor() {}
  ~vtkOpenGLActor() {}

private:
  vtkOpenGLActor(const vtkOpenGLActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLActor&) VTK_DELETE_FUNCTION;
};

#endif
