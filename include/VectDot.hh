/*=========================================================================

  Program:   Visualization Library
  Module:    VectDot.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlVectorDot - generate scalars from dot product of vectors and normals (e.g., show displacement plot)
// .SECTION Description
// vlVectorDot is a filter to generate scalar values from a dataset.
// The scalar value at a point is created by computing the dot product 
// between the normal and vector at that point. Combined with the appropriate
// color map, this can show nodal lines/mode shapes of vibration, or a 
// displacement plot.

#ifndef __vlVectorDot_h
#define __vlVectorDot_h

#include "DS2DSF.hh"

class vlVectorDot : public vlDataSetToDataSetFilter 
{
public:
  vlVectorDot();
  char *GetClassName() {return "vlVectorDot";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify range to map scalars into.
  vlSetVector2Macro(ScalarRange,float);
  // Description:
  // Get the range that scalars map into.
  vlGetVectorMacro(ScalarRange,float,2);

protected:
  void Execute();
  float ScalarRange[2];
};

#endif


