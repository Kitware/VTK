/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ShrinkP.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkShrinkPolyData - shrink cells composing PolyData
// .SECTION Description
// vtkShrinkPolyData shrinks cells composing a polygonal dataset (e.g., 
// vertices, lines, polygons, and triangle strips) towards their centroid. 
// The centroid of a cell is computed as the average position of the
// cell points. Shrinking results in disconencting the cells from
// one another.
// .SECTION Caveats
// It is possible to turn cells inside out or cause self intersection
// in special cases.

#ifndef __vtkShrinkPolyData_h
#define __vtkShrinkPolyData_h

#include "P2PF.hh"

class vtkShrinkPolyData : public vtkPolyToPolyFilter 
{
public:
  vtkShrinkPolyData(float sf=0.5) {this->ShrinkFactor = sf;};
  ~vtkShrinkPolyData() {};
  char *GetClassName() {return "vtkShrinkPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the fraction of shrink for each cell.
  vtkSetClampMacro(ShrinkFactor,float,0.0,1.0);

  // Description:
  // Get the fraction of shrink for each cell.
  vtkGetMacro(ShrinkFactor,float);

protected:
  void Execute();
  float ShrinkFactor;
};

#endif
