/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VectDot.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkVectorDot - generate scalars from dot product of vectors and normals (e.g., show displacement plot)
// .SECTION Description
// vtkVectorDot is a filter to generate scalar values from a dataset.
// The scalar value at a point is created by computing the dot product 
// between the normal and vector at that point. Combined with the appropriate
// color map, this can show nodal lines/mode shapes of vibration, or a 
// displacement plot.

#ifndef __vtkVectorDot_h
#define __vtkVectorDot_h

#include "DS2DSF.hh"

class vtkVectorDot : public vtkDataSetToDataSetFilter 
{
public:
  vtkVectorDot();
  char *GetClassName() {return "vtkVectorDot";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify range to map scalars into.
  vtkSetVector2Macro(ScalarRange,float);
  // Description:
  // Get the range that scalars map into.
  vtkGetVectorMacro(ScalarRange,float,2);

protected:
  void Execute();
  float ScalarRange[2];
};

#endif


