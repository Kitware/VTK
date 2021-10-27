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

#include "vtkNew.h"                   // ivars
#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkTransform.h"             // for method
#include "vtkVRCamera.h"

class vtkOpenXRRenderWindow;
class vtkVRRenderWindow;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRCamera : public vtkVRCamera
{
public:
  static vtkOpenXRCamera* New();
  vtkTypeMacro(vtkOpenXRCamera, vtkVRCamera);

  /**
   * Provides a matrix to go from absolute VR tracking coordinates
   * to device coordinates. Used for rendering devices.
   */
  void GetTrackingToDCMatrix(vtkMatrix4x4*& TCDCMatrix) override;

  void GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& WCVCMatrix, vtkMatrix3x3*& normalMatrix,
    vtkMatrix4x4*& VCDCMatrix, vtkMatrix4x4*& WCDCMatrix) override;

  // Get the VR Physical Space to World coordinate matrix
  vtkTransform* GetPhysicalToWorldTransform() { return this->PoseTransform.Get(); }

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

protected:
  vtkOpenXRCamera();
  ~vtkOpenXRCamera() override = default;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // gets the pose and projections for the left and right eyes from
  // the openvr library
  void GetHMDEyePoses(vtkRenderer*);
  void GetHMDEyeProjections(vtkRenderer*);

  vtkNew<vtkMatrix4x4> LeftEyeProjection;
  vtkNew<vtkMatrix4x4> RightEyeProjection;
  vtkNew<vtkMatrix4x4> LeftEyeView;
  vtkNew<vtkMatrix4x4> RightEyeView;
  vtkNew<vtkMatrix4x4> LeftEyeTCDCMatrix;
  vtkNew<vtkMatrix4x4> RightEyeTCDCMatrix;

  // used to translate the
  // View to the HMD space
  vtkNew<vtkTransform> PoseTransform;

private:
  vtkOpenXRCamera(const vtkOpenXRCamera&) = delete;
  void operator=(const vtkOpenXRCamera&) = delete;
};

#endif
