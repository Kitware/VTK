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
// .NAME vtkOpenGLActor - OpenGL actor
// .SECTION Description
// vtkOpenGLActor is a concrete implementation of the abstract class vtkActor.
// vtkOpenGLActor interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGL2Actor_h
#define __vtkOpenGL2Actor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkActor.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Actor : public vtkActor
{
public:
  static vtkOpenGL2Actor *New();
  vtkTypeMacro(vtkOpenGL2Actor, vtkActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actual actor render method.
  void Render(vtkRenderer *ren, vtkMapper *mapper);

protected:
  vtkOpenGL2Actor() {}
  ~vtkOpenGL2Actor() {}

private:
  vtkOpenGL2Actor(const vtkOpenGL2Actor&);  // Not implemented.
  void operator=(const vtkOpenGL2Actor&);  // Not implemented.
};

#endif
