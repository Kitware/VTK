/*=========================================================================

  Program:   Visualization Library
  Module:    DCubes.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Dividing cubes algorithm
//
#ifndef __vlDividingCubes_h
#define __vlDividingCubes_h

#include "SPt2Poly.hh"

class vlDividingCubes : public vlStructuredPointsToPolyDataFilter
{
public:
  vlDividingCubes();
  ~vlDividingCubes() {};
  char *GetClassName() {return "vlDividingCubes";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetMacro(Value,float);
  vlGetMacro(Value,float);

  vlSetClampMacro(Distance,float,1.0e-06,LARGE_FLOAT);
  vlGetMacro(Distance,float);

protected:
  void Execute();
  void SubDivide(float pMin[3], float pMax[3], float values[8], 
                 float distance2, vlCell& cell);
  void AddPoint(float pcoords[3], vlCell& cell);

  float Value;
  float Distance;
};

#endif


