/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPicker.h
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
// .NAME vtkCellPicker - select a cell by shooting a ray into graphics window
// .SECTION Description
// vtkCellPicker is used to select a cell by shooting a ray into graphics
// window and intersecting with actor's defining geometry - specifically 
// its cells. Beside returning coordinates, actor and mapper, vtkCellPicker
// returns the id of the closest cell within the tolerance along the pick
// ray, and the dataset that was picked.
// .SECTION See Also
// vtkPicker vtkPointPicker

#ifndef __vtkCellPicker_h
#define __vtkCellPicker_h

#include "vtkPicker.h"

class vtkGenericCell;

class VTK_RENDERING_EXPORT vtkCellPicker : public vtkPicker
{
public:
  static vtkCellPicker *New();
  vtkTypeRevisionMacro(vtkCellPicker,vtkPicker);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the id of the picked cell. If CellId = -1, nothing was picked.
  vtkGetMacro(CellId, vtkIdType);

  // Description:
  // Get the subId of the picked cell. If SubId = -1, nothing was picked.
  vtkGetMacro(SubId, int);

  // Description:
  // Get the parametric coordinates of the picked cell. Only valid if 
  // pick was made.
  vtkGetVectorMacro(PCoords, float,3);

protected:
  vtkCellPicker();
  ~vtkCellPicker();

  vtkIdType CellId; // picked cell
  int SubId; // picked cell subId
  float PCoords[3]; // picked cell parametric coordinates

  virtual float IntersectWithLine(float p1[3], float p2[3], float tol, 
                                  vtkAssemblyPath *path, vtkProp3D *p, 
                                  vtkAbstractMapper3D *m);
  void Initialize();
  
private:
  vtkGenericCell *Cell; //used to accelerate picking
  
private:
  vtkCellPicker(const vtkCellPicker&);  // Not implemented.
  void operator=(const vtkCellPicker&);  // Not implemented.
};

#endif


