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
 * @class   vtkVRCamera
 * @brief   VR camera
 *
 * vtkVRCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkVRCamera interfaces to the VR rendering library.
 */

#ifndef vtkVRCamera_h
#define vtkVRCamera_h

#include "vtkDeprecation.h" // for VTK_DEPRECATED_IN_9_2_0
#include "vtkNew.h"         // for iavr
#include "vtkOpenGLCamera.h"
#include "vtkRenderingVRModule.h" // For export macro

class vtkMatrix4x4;
class vtkVRRenderWindow;

class VTKRENDERINGVR_EXPORT vtkVRCamera : public vtkOpenGLCamera
{
public:
  vtkTypeMacro(vtkVRCamera, vtkOpenGLCamera);

  /**
   * Provides a matrix to go from physical coordinates to projection coordinates
   * for the eye currently being rendered. Just e.g. LeftEyeToProjection *
   * PhysicalToLeftEye
   */
  VTK_DEPRECATED_IN_9_2_0("use GetPhysicalToProjectionMatrix instead")
  virtual void GetTrackingToDCMatrix(vtkMatrix4x4*& physicalToProjectionMatrix)
  {
    this->GetPhysicalToProjectionMatrix(physicalToProjectionMatrix);
  };
  virtual void GetPhysicalToProjectionMatrix(vtkMatrix4x4*& physicalToProjectionMatrix) = 0;

  // A pose in VR includes more than just the basic camera values.
  // It includes all the properties needed to reproduce a view
  // in physical space when requested from a different physical space
  // This class stores those properties
  // As the VR code is still being rearchitected this signature
  // may change slightly through the end of 2021.
  class Pose
  {
  public:
    double Position[3];
    double PhysicalViewUp[3];
    double PhysicalViewDirection[3];
    double ViewDirection[3];
    double Translation[3];
    double Distance;
    double MotionFactor = 1.0;
  };

  // Fill in a Pose object based on the current camera and physical space
  // settings. As the VR code is still being rearchitected this signature
  // may change slightly through the end of 2021.
  void SetPoseFromCamera(Pose* pose, vtkVRRenderWindow* win);

  // Reproduce a pose using the current camera and render window. That is, try
  // to make the viewer's current view look like the original saved pose.
  // This is complicated by the fact that the viewer may now occupy a very
  // different position and orientation in the physical space than when the
  // pose was saved. This method accounts for this and adjusts the phjsical
  // space to best fit the requested pose.
  // As the VR code is still being rearchitected this signature
  // may change slightly through the end of 2021.
  void ApplyPoseToCamera(Pose* pose, vtkVRRenderWindow* win);

  // Set the camera's ivars based on a user provided matrix. The goal here
  // is to make it so that the camera is consistent with the provided matrix
  // and when the world to pose/view matrix is requested would return the
  // same matrix as provided.
  void SetCameraFromWorldToDeviceMatrix(vtkMatrix4x4* mat, double distance);
  void SetCameraFromDeviceToWorldMatrix(vtkMatrix4x4* mat, double distance);

protected:
  vtkVRCamera();
  ~vtkVRCamera() override;

  vtkNew<vtkMatrix4x4> TempMatrix4x4;

private:
  vtkVRCamera(const vtkVRCamera&) = delete;
  void operator=(const vtkVRCamera&) = delete;
};

#endif
