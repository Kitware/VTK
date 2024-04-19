// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRHMDCamera.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENXR_EXPORT vtkOpenXRCamera : public vtkVRHMDCamera
{
public:
  static vtkOpenXRCamera* New();
  vtkTypeMacro(vtkOpenXRCamera, vtkVRHMDCamera);

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

protected:
  vtkOpenXRCamera();
  ~vtkOpenXRCamera() override;

  // gets the pose and projections for the left and right eyes from
  // the openvr library
  void UpdateWorldToEyeMatrices(vtkRenderer*) override;
  void UpdateEyeToProjectionMatrices(vtkRenderer*) override;

private:
  vtkOpenXRCamera(const vtkOpenXRCamera&) = delete;
  void operator=(const vtkOpenXRCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
