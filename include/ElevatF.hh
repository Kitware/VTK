/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ElevatF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vtkElevationFilter is a filter to generate scalar values from a dataset.
// The scalar values lie within a user specified range, and are generated
// by computing a projection of each dataset point onto a line. The line
// can be oriented arbitrarily. A typical example is to generate scalars
// based on elevation or height above a plane.

#ifndef __vtkElevationFilter_h
#define __vtkElevationFilter_h

#include "DS2DSF.hh"

class vtkElevationFilter : public vtkDataSetToDataSetFilter 
{
public:
  vtkElevationFilter();
  char *GetClassName() {return "vtkElevationFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Define one end of the line (small scalar values).
  vtkSetVector3Macro(LowPoint,float);
  vtkGetVectorMacro(LowPoint,float,3);

  // Description:
  // Define other end of the line (large scalar values).
  vtkSetVector3Macro(HighPoint,float);
  vtkGetVectorMacro(HighPoint,float,3);

  // Description:
  // Specify range to map scalars into.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

protected:
  void Execute();
  float LowPoint[3];
  float HighPoint[3];
  float ScalarRange[2];
};

#endif


