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

#define VTK_USE_X_MIN 0
#define VTK_USE_Y_MIN 1
#define VTK_USE_Z_MIN 2
#define VTK_USE_X_MAX 3
#define VTK_USE_Y_MAX 4
#define VTK_USE_Z_MAX 5

class VTK_GRAPHICS_EXPORT vtkReflectionFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkReflectionFilter *New();
  
  vtkTypeRevisionMacro(vtkReflectionFilter, vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  vtkSetClampMacro(Plane, int, 0, 5);
  vtkGetMacro(Plane, int);
  void SetPlaneToXMin() { this->SetPlane(VTK_USE_X_MIN); };
  void SetPlaneToYMin() { this->SetPlane(VTK_USE_Y_MIN); };
  void SetPlaneToZMin() { this->SetPlane(VTK_USE_Z_MIN); };
  void SetPlaneToXMax() { this->SetPlane(VTK_USE_X_MAX); };
  void SetPlaneToYMax() { this->SetPlane(VTK_USE_Y_MAX); };
  void SetPlaneToZMax() { this->SetPlane(VTK_USE_Z_MAX); };
  
protected:
  vtkReflectionFilter();
  ~vtkReflectionFilter();
  
  void Execute();

  int Plane;
  
private:
  vtkReflectionFilter(const vtkReflectionFilter&);  // Not implemented
  void operator=(const vtkReflectionFilter&);  // Not implemented
};

#endif


