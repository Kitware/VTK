/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtPicker.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPointPicker - select a point by shooting a ray into graphics window
// .SECTION Description
// vtkPointPicker is used to select a point by shooting a ray into graphics
// window and intersecting with actor's defining geometry - specifically 
// its points. Beside returning coordinates, actor and mapper, vtkPointPicker
// returns the id of the closest point within the tolerance along the pick ray.
// .SECTION See Also
// For quick picking, see vtkPicker. To uniquely pick actors, see vtkCellPicker.

#ifndef __vtkPointPicker_h
#define __vtkPointPicker_h

#include "Picker.hh"

class vtkPointPicker : public vtkPicker
{
public:
  vtkPointPicker();
  ~vtkPointPicker() {};
  char *GetClassName() {return "vtkPointPicker";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked point. If PointId = -1, nothing was picked.
  vtkGetMacro(PointId,int);

protected:
  int PointId; //picked point

  void IntersectWithLine(float p1[3], float p2[3], float tol, 
                         vtkActor *a, vtkMapper *m);
  void Initialize();

};

#endif


