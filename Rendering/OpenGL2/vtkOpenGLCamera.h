/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLCamera - OpenGL camera
// .SECTION Description
// vtkOpenGLCamera is a concrete implementation of the abstract class
// vtkCamera.  vtkOpenGLCamera interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGL2Camera_h
#define __vtkOpenGL2Camera_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkCamera.h"

class vtkOpenGL2Renderer;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2Camera : public vtkCamera
{
public:
  static vtkOpenGL2Camera *New();
  vtkTypeMacro(vtkOpenGL2Camera, vtkCamera);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren);

  void UpdateViewport(vtkRenderer *ren);

protected:
  vtkOpenGL2Camera() {}
  ~vtkOpenGL2Camera() {}
private:
  vtkOpenGL2Camera(const vtkOpenGL2Camera&);  // Not implemented.
  void operator=(const vtkOpenGL2Camera&);  // Not implemented.
};

#endif
