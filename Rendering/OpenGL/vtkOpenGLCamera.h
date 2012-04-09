/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.h

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

#ifndef __vtkOpenGLCamera_h
#define __vtkOpenGLCamera_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCamera.h"

class vtkOpenGLRenderer;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLCamera : public vtkCamera
{
public:
  static vtkOpenGLCamera *New();
  vtkTypeMacro(vtkOpenGLCamera,vtkCamera);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren);

  void UpdateViewport(vtkRenderer *ren);

protected:
  vtkOpenGLCamera() {};
  ~vtkOpenGLCamera() {};
private:
  vtkOpenGLCamera(const vtkOpenGLCamera&);  // Not implemented.
  void operator=(const vtkOpenGLCamera&);  // Not implemented.
};

#endif
