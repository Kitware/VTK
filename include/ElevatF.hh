/*=========================================================================

  Program:   Visualization Library
  Module:    ElevatF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vlElevationFilter is a filter to generate scalar values from a dataset.
// The scalar values lie within a user specified range, and are generated
// by computing a projection of each dataset point onto a line. The line
// can be oriented arbitrarily. A typical example is to generate scalars
// based on elevation or height above a plane.

#ifndef __vlElevationFilter_h
#define __vlElevationFilter_h

#include "DS2DSF.hh"

class vlElevationFilter : public vlDataSetToDataSetFilter 
{
public:
  vlElevationFilter();
  char *GetClassName() {return "vlElevationFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Define one end of the line (small scalar values).
  vlSetVector3Macro(LowPoint,float);
  vlGetVectorMacro(LowPoint,float);

  // Description:
  // Define other end of the line (large scalar values).
  vlSetVector3Macro(HighPoint,float);
  vlGetVectorMacro(HighPoint,float);

  // Description:
  // Specify range to map scalars into.
  vlSetVector2Macro(ScalarRange,float);
  vlGetVectorMacro(ScalarRange,float);

protected:
  void Execute();
  float LowPoint[3];
  float HighPoint[3];
  float ScalarRange[2];
};

#endif


