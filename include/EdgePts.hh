/*=========================================================================

  Program:   Visualization Toolkit
  Module:    EdgePts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkEdgePoints - generate points on iso-surface
// .SECTION Description
// vtkEdgePoints is a filter that takes as input any dataset and 
// generates for output a set of points that lie on an iso-surface. The 
// points are created by interpolation along cells edges whose end-points are 
// below and above the contour value.
// .SECTION Caveats
// vtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
// (see vtkDividingCubes). Points are generated only on the edges of cells, 
// not in the interior, and at lower density than dividing cubes. However, it 
// is more general than dividing cubes since it treats any type of dataset.

#ifndef __vtkEdgePoints_h
#define __vtkEdgePoints_h

#include "DS2PolyF.hh"

class vtkEdgePoints : public vtkDataSetToPolyFilter
{
public:
  vtkEdgePoints();
  ~vtkEdgePoints();
  char *GetClassName() {return "vtkEdgePoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the contour value.
  vtkSetMacro(Value,float);
  vtkGetMacro(Value,float);

protected:
  void Execute();

  float Value;
};

#endif


