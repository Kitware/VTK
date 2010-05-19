/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointPicker - select a point by shooting a ray into a graphics window
// .SECTION Description

// vtkPointPicker is used to select a point by shooting a ray into a graphics
// window and intersecting with actor's defining geometry - specifically its
// points. Beside returning coordinates, actor, and mapper, vtkPointPicker
// returns the id of the point projecting closest onto the ray (within the
// specified tolerance).  Ties are broken (i.e., multiple points all
// projecting within the tolerance along the pick ray) by choosing the point
// closest to the ray.
// 

// .SECTION See Also
// vtkPicker vtkCellPicker.

#ifndef __vtkPointPicker_h
#define __vtkPointPicker_h

#include "vtkPicker.h"

class VTK_RENDERING_EXPORT vtkPointPicker : public vtkPicker
{
public:
  static vtkPointPicker *New();
  vtkTypeMacro(vtkPointPicker,vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked point. If PointId = -1, nothing was picked.
  vtkGetMacro(PointId, vtkIdType);

protected:
  vtkPointPicker();
  ~vtkPointPicker() {};

  vtkIdType PointId; //picked point

  double IntersectWithLine(double p1[3], double p2[3], double tol, 
                          vtkAssemblyPath *path, vtkProp3D *p, 
                          vtkAbstractMapper3D *m);
  void Initialize();

private:
  vtkPointPicker(const vtkPointPicker&);  // Not implemented.
  void operator=(const vtkPointPicker&);  // Not implemented.
};

#endif


