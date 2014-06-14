/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGL2Light - OpenGL light
// .SECTION Description
// vtkOpenGL2Light is a concrete implementation of the abstract class vtkLight.
// vtkOpenGL2Light interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGL2Light_h
#define __vtkOpenGL2Light_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkLight.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Light : public vtkLight
{
public:
  static vtkOpenGL2Light *New();
  vtkTypeMacro(vtkOpenGL2Light, vtkLight);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren, int light_index);

protected:
  vtkOpenGL2Light() {}
  ~vtkOpenGL2Light() {}

private:
  vtkOpenGL2Light(const vtkOpenGL2Light&);  // Not implemented.
  void operator=(const vtkOpenGL2Light&);  // Not implemented.
};

#endif
