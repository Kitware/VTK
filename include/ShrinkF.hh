/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ShrinkF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkShrinkFilter - shrink cells composing an arbitrary data set
// .SECTION Description
// vtkShrinkFilter shrinks cells composing an arbitrary data set 
// towards their centroid. The centroid of a cell is computed as 
// the average position of the cell points. Shrinking results in 
// disconencting the cells from one another.
// .SECTION Caveats
// It is possible to turn cells inside out or cause self intersection
// in special cases.

#ifndef __vtkShrinkFilter_h
#define __vtkShrinkFilter_h

#include "DS2UGrid.hh"

class vtkShrinkFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkShrinkFilter(float sf=0.5) {this->ShrinkFactor = sf;};
  ~vtkShrinkFilter() {};
  char *GetClassName() {return "vtkShrinkFilter";};
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


