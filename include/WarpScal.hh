/*=========================================================================

  Program:   Visualization Library
  Module:    WarpScal.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlWarpScalar - deform geometry with scalar data
// .SECTION Description
// vlWarpScalar is a filter that modifies point coordinates by moving
// points along point normals by the scalar amount times the scale factor.
// Useful for creating carpet or x-y-z plots.

#ifndef __vlWarpScalar_h
#define __vlWarpScalar_h

#include "PtS2PtSF.hh"

class vlWarpScalar : public vlPointSetToPointSetFilter
{
public:
  vlWarpScalar() : ScaleFactor(1.0) {};
  ~vlWarpScalar() {};
  char *GetClassName() {return "vlWarpScalar";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify value to scale displacement.
  vlSetMacro(ScaleFactor,float);
  vlGetMacro(ScaleFactor,float);

protected:
  void Execute();
  float ScaleFactor;
};

#endif


