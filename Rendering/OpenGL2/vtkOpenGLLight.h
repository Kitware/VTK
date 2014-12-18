/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLLight - OpenGL light
// .SECTION Description
// vtkOpenGLLight is a concrete implementation of the abstract class vtkLight.
// vtkOpenGLLight interfaces to the OpenGL rendering library.

#ifndef vtkOpenGLLight_h
#define vtkOpenGLLight_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkLight.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLLight : public vtkLight
{
public:
  static vtkOpenGLLight *New();
  vtkTypeMacro(vtkOpenGLLight, vtkLight);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren, int light_index);

protected:
  vtkOpenGLLight() {}
  ~vtkOpenGLLight() {}

private:
  vtkOpenGLLight(const vtkOpenGLLight&);  // Not implemented.
  void operator=(const vtkOpenGLLight&);  // Not implemented.
};

#endif
