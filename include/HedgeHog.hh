/*=========================================================================

  Program:   Visualization Toolkit
  Module:    HedgeHog.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkHedgeHog - create oriented lines from vector data
// .SECTION Description
// vtkHedgeHog creates oriented lines from the input data set. Line length
// is controlled by vector magnitude times scale factor. Vectors are
// colored by scalar data, if available.

#ifndef __vtkHedgeHog_h
#define __vtkHedgeHog_h

#include "DS2PolyF.hh"

class vtkHedgeHog : public vtkDataSetToPolyFilter
{
public:
  vtkHedgeHog() : ScaleFactor(1.0) {};
  ~vtkHedgeHog() {};
  char *GetClassName() {return "vtkHedgeHog";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set scale factor to control size of oriented lines.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;

};

#endif


