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

#ifndef vtkPointPicker_h
#define vtkPointPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPicker.h"

class VTKRENDERINGCORE_EXPORT vtkPointPicker : public vtkPicker
{
public:
  static vtkPointPicker *New();
  vtkTypeMacro(vtkPointPicker,vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked point. If PointId = -1, nothing was picked.
  vtkGetMacro(PointId, vtkIdType);

  // Description:
  // Specify whether the point search should be based on cell points or
  // directly on the point list.
  vtkSetMacro(UseCells, int);
  vtkGetMacro(UseCells, int);
  vtkBooleanMacro(UseCells, int);

protected:
  vtkPointPicker();
  ~vtkPointPicker() {}

  vtkIdType PointId; //picked point
  int UseCells;  // Use cell points vs. points directly

  double IntersectWithLine(double p1[3], double p2[3], double tol,
                          vtkAssemblyPath *path, vtkProp3D *p,
                          vtkAbstractMapper3D *m);
  void Initialize();

private:
  vtkPointPicker(const vtkPointPicker&);  // Not implemented.
  void operator=(const vtkPointPicker&);  // Not implemented.
};

#endif


