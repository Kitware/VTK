/*=========================================================================

  Program:   Visualization Library
  Module:    ImpMod.hh
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
// Generate implicit volume model from PolyData
//
#ifndef __vlImplicitModeller_h
#define __vlImplicitModeller_h

#include "DS2SPtsF.hh"

class vlImplicitModeller : public vlDataSetToStructuredPointsFilter 
{
public:
  vlImplicitModeller();
  ~vlImplicitModeller() {};
  char *GetClassName() {return "vlImplicitModeller";};
  void PrintSelf(ostream& os, vlIndent indent);

  float ComputeModelBounds();

  vlSetClampMacro(MaximumDistance,float,0.0,1.0);
  vlGetMacro(MaximumDistance,float);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

protected:
  void Execute();
  float MaximumDistance;
  float ModelBounds[6];
};

#endif


