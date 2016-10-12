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
 * @class   vtkOculusCamera
 * @brief   Oculus camera
 *
 * vtkOculusCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkOculusCamera interfaces to the Oculus rendering library.
*/

#ifndef vtkOculusCamera_h
#define vtkOculusCamera_h

#include "vtkRenderingOculusModule.h" // For export macro
#include "vtkOpenGLCamera.h"
#include "vtkNew.h" // ivars
#include "vtkTransform.h" // ivars

class vtkOculusRenderer;
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGOCULUS_EXPORT vtkOculusCamera : public vtkOpenGLCamera
{
public:
  static vtkOculusCamera *New();
  vtkTypeMacro(vtkOculusCamera, vtkOpenGLCamera);

  /**
   * Implement base class method.
   */
  virtual void Render(vtkRenderer *ren);

  virtual void GetKeyMatrices(vtkRenderer *ren, vtkMatrix4x4 *&WCVCMatrix,
    vtkMatrix3x3 *&normalMatrix, vtkMatrix4x4 *&VCDCMatrix, vtkMatrix4x4 *&WCDCMatrix);

  /**
   * Provides a matrix to go from absolute Oculus tracking coordinates
   * to device coordinates. Used for rendering devices.
   */
  virtual void GetTrackingToDCMatrix(vtkMatrix4x4 *&TCDCMatrix);

  //@{
  /**
   * Set/Get the translation to map world coordinates into the
   * Oculus physical space (meters, 0,0,0).
   */
  vtkSetVector3Macro(Translation,double);
  vtkGetVector3Macro(Translation,double);
  //@}

protected:
  vtkOculusCamera();
  ~vtkOculusCamera();

  // gets the pose and projections for the left and right eves from
  // the Oculus library
  void GetHMDEyePoses(vtkRenderer *);
  void GetHMDEyeProjections(vtkRenderer *);

  vtkMatrix4x4 *LeftEyePose;
  vtkMatrix4x4 *RightEyePose;
  vtkMatrix4x4 *LeftEyeProjection;
  vtkMatrix4x4 *RightEyeProjection;

  vtkMatrix4x4 *RightWCDCMatrix;
  vtkMatrix4x4 *RightWCVCMatrix;
  vtkMatrix4x4 *RightVCDCMatrix;

  vtkMatrix4x4 *LeftEyeTCDCMatrix;
  vtkMatrix4x4 *RightEyeTCDCMatrix;

  // used to scale and/or translate the
  // View to the HMD space
  double Translation[3];
  vtkNew<vtkTransform> PoseTransform;

private:
  vtkOculusCamera(const vtkOculusCamera&);  // Not implemented.
  void operator=(const vtkOculusCamera&);  // Not implemented.
};

#endif
