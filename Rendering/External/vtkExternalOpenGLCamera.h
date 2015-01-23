/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExternalOpenGLCamera - OpenGL camera
// .SECTION Description
// vtkExternalOpenGLCamera is a concrete implementation of the abstract class
// vtkCamera.  vtkExternalOpenGLCamera interfaces to the OpenGL rendering library.
// This class extends vtkOpenGLCamera by introducing API wherein the camera
// matrices can be set explicitly by the application.

#ifndef __vtkExternalOpenGLCamera_h
#define __vtkExternalOpenGLCamera_h

#include "vtkRenderingExternalModule.h" // For export macro
#include "vtkOpenGLCamera.h"

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalOpenGLCamera :
  public vtkOpenGLCamera
{
public:
  static vtkExternalOpenGLCamera *New();
  vtkTypeMacro(vtkExternalOpenGLCamera, vtkOpenGLCamera);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren);

  // Description:
  // Set the view transform matrix
  void SetViewTransformMatrix(const double elements[16]);

  // Description:
  // Set the projection matrix
  void SetProjectionTransformMatrix(const double elements[16]);

protected:
  vtkExternalOpenGLCamera();
  ~vtkExternalOpenGLCamera() {}

  // Description:
  // These methods should only be used within vtkCamera.cxx.
  // Bypass computation if user provided the projection transform
  void ComputeProjectionTransform(double aspect,
                                  double nearz,
                                  double farz);

  // Description:
  // These methods should only be used within vtkCamera.cxx.
  // Bypass computation if user provided the view transform
  void ComputeViewTransform();

private:
  bool UserProvidedProjectionTransform;
  bool UserProvidedViewTransform;

  vtkExternalOpenGLCamera(const vtkExternalOpenGLCamera&);  // Not implemented.
  void operator=(const vtkExternalOpenGLCamera&);  // Not implemented.
};

#endif
