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
/**
 * @class   vtkExternalOpenGLCamera
 * @brief   OpenGL camera
 *
 * vtkExternalOpenGLCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkExternalOpenGLCamera interfaces to the OpenGL rendering library.
 * This class extends vtkOpenGLCamera by introducing API wherein the camera
 * matrices can be set explicitly by the application.
 */

#ifndef vtkExternalOpenGLCamera_h
#define vtkExternalOpenGLCamera_h

#include "vtkOpenGLCamera.h"
#include "vtkRenderingExternalModule.h" // For export macro

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalOpenGLCamera : public vtkOpenGLCamera
{
public:
  static vtkExternalOpenGLCamera* New();
  vtkTypeMacro(vtkExternalOpenGLCamera, vtkOpenGLCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the view transform matrix
   */
  void SetViewTransformMatrix(const double elements[16]);

  /**
   * Set the projection matrix
   */
  void SetProjectionTransformMatrix(const double elements[16]);

protected:
  vtkExternalOpenGLCamera();
  ~vtkExternalOpenGLCamera() override {}

  /**
   * These methods should only be used within vtkCamera.cxx.
   * Bypass computation if user provided the view transform
   */
  void ComputeViewTransform() override;

private:
  bool UserProvidedViewTransform;

  vtkExternalOpenGLCamera(const vtkExternalOpenGLCamera&) = delete;
  void operator=(const vtkExternalOpenGLCamera&) = delete;
};

#endif
