/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRCamera
 * @brief   OpenXR camera
 *
 * vtkOpenXRCamera is a concrete implementation of the abstract class
 * vtkCamera.
 *
 * vtkOpenXRCamera interfaces to the OpenXR rendering library.
 *
 * It sets a custom view transform and projection matrix from the view pose and projection
 * fov given by vtkOpenXRManager
 */

#ifndef vtkOpenXRCamera_h
#define vtkOpenXRCamera_h

#include "vtkNew.h" // ivars
#include "vtkOpenGLCamera.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

class vtkOpenXRRenderWindow;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRCamera : public vtkOpenGLCamera
{
public:
  static vtkOpenXRCamera* New();
  vtkTypeMacro(vtkOpenXRCamera, vtkOpenGLCamera);

  /**
   * Implement base class method.
   */
  virtual void Render(vtkRenderer* ren) override;

protected:
  vtkOpenXRCamera();
  ~vtkOpenXRCamera() override = default;

  void UpdateViewTransform(vtkOpenXRRenderWindow*);
  void UpdateProjectionMatrix();

private:
  vtkOpenXRCamera(const vtkOpenXRCamera&) = delete;
  void operator=(const vtkOpenXRCamera&) = delete;
};

#endif
