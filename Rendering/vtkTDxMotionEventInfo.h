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

#ifndef __vtkTDxMotionEventInfo_h
#define __vtkTDxMotionEventInfo_h

#include "vtkObject.h" // for the export macro

class VTK_RENDERING_EXPORT vtkTDxMotionEventInfo
{
public:
  // Description:
  // Translation coordinates (deltas)
  double X;
  double Y;
  double Z;
  
  // Description:
  // Rotation angles (deltas)
  double A;
  double B;
  double C;
};

#endif
