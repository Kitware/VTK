/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxMotionEventInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTDxMotionEventInfo - Store motion information from a 3DConnexion input device
// .SECTION Description
// vtkTDxMotionEventInfo is a data structure that stores the information about
// a motion event from a 3DConnexion input device.
//
// .SECTION See Also
// vtkTDxDevice

#ifndef vtkTDxMotionEventInfo_h
#define vtkTDxMotionEventInfo_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h" // for the export macro

class VTKRENDERINGCORE_EXPORT vtkTDxMotionEventInfo
{
public:
  // Description:
  // Translation coordinates
  double X;
  double Y;
  double Z;

  // Description:
  // Rotation angle.
  // The angle is in arbitrary unit.
  // It makes sense to have arbitrary unit
  // because the data comes from a device
  // where the information can be scaled by
  // the end-user.
  double Angle;

  // Description:
  // Rotation axis expressed as a unit vector.
  double AxisX;
  double AxisY;
  double AxisZ;
};

#endif
// VTK-HeaderTest-Exclude: vtkTDxMotionEventInfo.h
