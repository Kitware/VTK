// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRHMDCamera
 * @brief   A superclass for HMD style cameras
 *
 */

#ifndef vtkVRHMDCamera_h
#define vtkVRHMDCamera_h

#include "vtkNew.h"               // ivars
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkVRCamera.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkMatrix4x4;

class VTKRENDERINGVR_EXPORT vtkVRHMDCamera : public vtkVRCamera
{
public:
  vtkTypeMacro(vtkVRHMDCamera, vtkVRCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

  void GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& WCVCMatrix, vtkMatrix3x3*& normalMatrix,
    vtkMatrix4x4*& VCDCMatrix, vtkMatrix4x4*& WCDCMatrix) override;

  /**
   * Provides a matrix to go from physical coordinates to projection coordinates
   * for the eye currently being rendered. Just e.g. LeftEyeToProjection *
   * PhysicalToLeftEye
   */
  void GetPhysicalToProjectionMatrix(vtkMatrix4x4*& physicalToProjectionMatrtix) override;

protected:
  vtkVRHMDCamera();
  ~vtkVRHMDCamera() override;

  // you must provide these two methods in your subclass
  virtual void UpdateWorldToEyeMatrices(vtkRenderer*) = 0;
  virtual void UpdateEyeToProjectionMatrices(vtkRenderer*) = 0;

  // all the matrices below are stored in VTK convention
  // as A = Mx where x is a column vector.

  // the physical to hmd (left and right eye) part
  vtkNew<vtkMatrix4x4> PhysicalToLeftEyeMatrix;
  vtkNew<vtkMatrix4x4> PhysicalToRightEyeMatrix;

  // adds in the world to physical part
  vtkNew<vtkMatrix4x4> WorldToLeftEyeMatrix;
  vtkNew<vtkMatrix4x4> WorldToRightEyeMatrix;

  // we get these from the VR system possibly with some modifications for
  // adjusting the clipping range or zbuffer formula
  vtkNew<vtkMatrix4x4> LeftEyeToProjectionMatrix;
  vtkNew<vtkMatrix4x4> RightEyeToProjectionMatrix;

  // computed using the above matrices, these matrices go from physical to
  // projection space but that transformation will be different depending on
  // which eye is active. So the naming is different as the start and end
  // space is not tied to an eye, but rather some of the internal
  // transformations
  vtkNew<vtkMatrix4x4> PhysicalToProjectionMatrixForLeftEye;
  vtkNew<vtkMatrix4x4> PhysicalToProjectionMatrixForRightEye;

  vtkNew<vtkMatrix4x4> WorldToPhysicalMatrix;

private:
  vtkVRHMDCamera(const vtkVRHMDCamera&) = delete;
  void operator=(const vtkVRHMDCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
