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

#ifndef vtkOpenGLActor_h
#define vtkOpenGLActor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkActor.h"

class vtkOpenGLRenderer;
class vtkMatrix4x4;
class vtkMatrix3x3;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLActor : public vtkActor
{
public:
  static vtkOpenGLActor *New();
  vtkTypeMacro(vtkOpenGLActor, vtkActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actual actor render method.
  void Render(vtkRenderer *ren, vtkMapper *mapper);

  void GetKeyMatrices(vtkMatrix4x4 *&WCVCMatrix, vtkMatrix3x3 *&normalMatrix);

protected:
  vtkOpenGLActor();
  ~vtkOpenGLActor();

  vtkMatrix4x4 *MCWCMatrix;
  vtkMatrix3x3 *NormalMatrix;
  vtkTransform *NormalTransform;
  vtkTimeStamp KeyMatrixTime;

private:
  vtkOpenGLActor(const vtkOpenGLActor&);  // Not implemented.
  void operator=(const vtkOpenGLActor&);  // Not implemented.
};

#endif
