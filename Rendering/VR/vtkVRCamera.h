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

#include "vtkOpenGLCamera.h"
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkTransform.h"         // ivars

class vtkMatrix4x4;

class VTKRENDERINGVR_EXPORT vtkVRCamera : public vtkOpenGLCamera
{
public:
  vtkTypeMacro(vtkVRCamera, vtkOpenGLCamera);

  /**
   * Provides a matrix to go from absolute VR tracking coordinates
   * to device coordinates. Used for rendering devices.
   */
  virtual void GetTrackingToDCMatrix(vtkMatrix4x4*& TCDCMatrix) = 0;

protected:
  vtkVRCamera() = default;
  ~vtkVRCamera() override = default;

private:
  vtkVRCamera(const vtkVRCamera&) = delete;
  void operator=(const vtkVRCamera&) = delete;
};

#endif
