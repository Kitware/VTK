/*=========================================================================

  Program:   Visualization Toolkit
  Module:    WarpScal.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkWarpScalar - deform geometry with scalar data
// .SECTION Description
// vtkWarpScalar is a filter that modifies point coordinates by moving
// points along point normals by the scalar amount times the scale factor.
// Useful for creating carpet or x-y-z plots.

#ifndef __vtkWarpScalar_h
#define __vtkWarpScalar_h

#include "PtS2PtSF.hh"

class vtkWarpScalar : public vtkPointSetToPointSetFilter
{
public:
  vtkWarpScalar() : ScaleFactor(1.0) {};
  ~vtkWarpScalar() {};
  char *GetClassName() {return "vtkWarpScalar";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify value to scale displacement.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;
};

#endif


