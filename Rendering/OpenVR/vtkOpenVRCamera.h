// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRCamera
 * @brief   OpenVR camera
 *
 * vtkOpenVRCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkOpenVRCamera interfaces to the OpenVR rendering library.
 */

#ifndef vtkOpenVRCamera_h
#define vtkOpenVRCamera_h

#include "vtkNew.h"                   // ivars
#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRHMDCamera.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkMatrix4x4;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRCamera : public vtkVRHMDCamera
{
public:
  static vtkOpenVRCamera* New();
  vtkTypeMacro(vtkOpenVRCamera, vtkVRHMDCamera);

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

protected:
  vtkOpenVRCamera();
  ~vtkOpenVRCamera() override;

  // gets the pose and projections for the left and right eyes from
  // the openvr library
  void UpdateHMDToEyeMatrices(vtkRenderer*);
  void UpdateWorldToEyeMatrices(vtkRenderer*) override;
  void UpdateEyeToProjectionMatrices(vtkRenderer*) override;

  // all the matrices below are stored in VTK convention
  // as A = Mx where x is a column vector.

  // we get these from OpenVR
  vtkNew<vtkMatrix4x4> HMDToLeftEyeMatrix;
  vtkNew<vtkMatrix4x4> HMDToRightEyeMatrix;

  // used as part of the calculation
  vtkNew<vtkMatrix4x4> PhysicalToHMDMatrix;

private:
  vtkOpenVRCamera(const vtkOpenVRCamera&) = delete;
  void operator=(const vtkOpenVRCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
