/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReflectionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReflectionFilter - reflects a data set across a plane
// .SECTION Description
// The vtkReflectionFilter reflects a data set across one of the
// planes formed by the data set's bounding box.
// Since it converts data sets into unstructured grids, it is not effeicient
// for structured data sets.

#ifndef __vtkReflectionFilter_h
#define __vtkReflectionFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkReflectionFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkReflectionFilter *New();
  
  vtkTypeRevisionMacro(vtkReflectionFilter, vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream &os, vtkIndent indent);
  
//BTX
  enum ReflectionPlane 
  {
    USE_X_MIN = 0,
    USE_Y_MIN = 1,
    USE_Z_MIN = 2,
    USE_X_MAX = 3,
    USE_Y_MAX = 4,
    USE_Z_MAX = 5,
    USE_X = 6,
    USE_Y = 7,
    USE_Z = 8
  };
//ETX

  // Description:
  // Set the normal of the plane to use as mirror.
  vtkSetClampMacro(Plane, int, 0, 8);
  vtkGetMacro(Plane, int);
  void SetPlaneToX() { this->SetPlane(USE_X); };
  void SetPlaneToY() { this->SetPlane(USE_Y); };
  void SetPlaneToZ() { this->SetPlane(USE_Z); };
  void SetPlaneToXMin() { this->SetPlane(USE_X_MIN); };
  void SetPlaneToYMin() { this->SetPlane(USE_Y_MIN); };
  void SetPlaneToZMin() { this->SetPlane(USE_Z_MIN); };
  void SetPlaneToXMax() { this->SetPlane(USE_X_MAX); };
  void SetPlaneToYMax() { this->SetPlane(USE_Y_MAX); };
  void SetPlaneToZMax() { this->SetPlane(USE_Z_MAX); };

  // Description:
  // If the reflection plane is set to X, Y or Z, this variable
  // is use to set the position of the plane.
  vtkSetMacro(Center, float);
  vtkGetMacro(Center, float);

protected:
  vtkReflectionFilter();
  ~vtkReflectionFilter();
  
  void Execute();

  int Plane;
  float Center;

  void FlipVector(float tuple[3], int mirrorDir[3]);
  
private:
  vtkReflectionFilter(const vtkReflectionFilter&);  // Not implemented
  void operator=(const vtkReflectionFilter&);  // Not implemented
};

#endif


