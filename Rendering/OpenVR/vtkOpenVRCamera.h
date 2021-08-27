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
#include "vtkTransform.h"             // ivars
#include "vtkVRCamera.h"

class vtkRenderer;
class vtkVRRenderWindow;
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRCamera : public vtkVRCamera
{
public:
  static vtkOpenVRCamera* New();
  vtkTypeMacro(vtkOpenVRCamera, vtkVRCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

  void GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& WCVCMatrix, vtkMatrix3x3*& normalMatrix,
    vtkMatrix4x4*& VCDCMatrix, vtkMatrix4x4*& WCDCMatrix) override;

  /**
   * Provides a matrix to go from absolute OpenVR tracking coordinates
   * to device coordinates. Used for rendering devices.
   */
  void GetTrackingToDCMatrix(vtkMatrix4x4*& TCDCMatrix) override;

  // apply the left or right eye pose to the camera
  // position and focal point.  Factor is typically
  // 1.0 to add or -1.0 to subtract
  void ApplyEyePose(vtkVRRenderWindow*, bool left, double factor);

  // Get the VR Physical Space to World coordinate matrix
  vtkTransform* GetPhysicalToWorldTransform() { return this->PoseTransform.Get(); }

protected:
  vtkOpenVRCamera();
  ~vtkOpenVRCamera() override;

  // gets the pose and projections for the left and right eyes from
  // the openvr library
  void GetHMDEyePoses(vtkRenderer*);
  void GetHMDEyeProjections(vtkRenderer*);

  double LeftEyePose[3];
  double RightEyePose[3];
  vtkMatrix4x4* LeftEyeProjection;
  vtkMatrix4x4* RightEyeProjection;

  vtkMatrix4x4* LeftEyeTCDCMatrix;
  vtkMatrix4x4* RightEyeTCDCMatrix;

  // used to translate the
  // View to the HMD space
  vtkNew<vtkTransform> PoseTransform;

private:
  vtkOpenVRCamera(const vtkOpenVRCamera&) = delete;
  void operator=(const vtkOpenVRCamera&) = delete;
};

#endif
