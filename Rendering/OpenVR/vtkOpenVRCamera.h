/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenVRCamera - OpenVR camera
// .SECTION Description
// vtkOpenVRCamera is a concrete implementation of the abstract class
// vtkCamera.  vtkOpenVRCamera interfaces to the OpenVR rendering library.

#ifndef vtkOpenVRCamera_h
#define vtkOpenVRCamera_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkOpenGLCamera.h"
#include "vtkNew.h" // ivars
#include "vtkTransform.h" // ivars

class vtkOpenVRRenderer;
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRCamera : public vtkOpenGLCamera
{
public:
  static vtkOpenVRCamera *New();
  vtkTypeMacro(vtkOpenVRCamera, vtkOpenGLCamera);

  // Description:
  // Implement base class method.
  virtual void Render(vtkRenderer *ren);

  virtual void GetKeyMatrices(vtkRenderer *ren, vtkMatrix4x4 *&WCVCMatrix,
    vtkMatrix3x3 *&normalMatrix, vtkMatrix4x4 *&VCDCMatrix, vtkMatrix4x4 *&WCDCMatrix);

  // Description:
  // Provides a matrix to go from absolute OpenVR tracking coordinates
  // to device coordinates. Used for rendering devices.
  virtual void GetTrackingToDCMatrix(vtkMatrix4x4 *&TCDCMatrix);

  // Description:
  // Set/Get the scale to map world coordinates into the
  // OpenVR physical space (meters).
  vtkSetMacro(Scale,double);
  vtkGetMacro(Scale,double);

  // Description:
  // Set/Get the translation to map world coordinates into the
  // OpenVR physical space (meters, 0,0,0).
  vtkSetVector3Macro(Translation,double);
  vtkGetVector3Macro(Translation,double);

protected:
  vtkOpenVRCamera();
  ~vtkOpenVRCamera();

  // gets the pose and projections for the left and right eves from
  // the openvr library
  void GetHMDEyePoses(vtkRenderer *);
  void GetHMDEyeProjections(vtkRenderer *);

  vtkMatrix4x4 *LeftEyePose;
  vtkMatrix4x4 *RightEyePose;
  vtkMatrix4x4 *LeftEyeProjection;
  vtkMatrix4x4 *RightEyeProjection;

  vtkMatrix4x4 *RightWCDCMatrix;
  vtkMatrix4x4 *RightWCVCMatrix;
  vtkMatrix3x3 *RightNormalMatrix;
  vtkMatrix4x4 *RightVCDCMatrix;

  vtkMatrix4x4 *LeftEyeTCDCMatrix;
  vtkMatrix4x4 *RightEyeTCDCMatrix;

  // used to scale and/or translate the
  // View to the HMD space
  double Scale;
  double Translation[3];
  vtkNew<vtkTransform> PoseTransform;

private:
  vtkOpenVRCamera(const vtkOpenVRCamera&);  // Not implemented.
  void operator=(const vtkOpenVRCamera&);  // Not implemented.
};

#endif
