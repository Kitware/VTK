/*=========================================================================

  Program:   Visualization Library
  Module:    TMap2Pl.hh
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
// Class generates scalar data from position of points along some ray
//
#ifndef __vlTextureMapToPlane_h
#define __vlTextureMapToPlane_h

#include "DS2DSF.hh"

class vlTextureMapToPlane : public vlDataSetToDataSetFilter 
{
public:
  vlTextureMapToPlane();
  char *GetClassName() {return "vlTextureMapToPlane";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetVector3Macro(Normal,float);
  vlGetVectorMacro(Normal,float);

  vlSetVector2Macro(SRange,float);
  vlGetVectorMacro(SRange,float);

  vlSetVector2Macro(TRange,float);
  vlGetVectorMacro(TRange,float);

  vlSetMacro(AutomaticNormalGeneration,int);
  vlGetMacro(AutomaticNormalGeneration,int);
  vlBooleanMacro(AutomaticNormalGeneration,int);

protected:
  void Execute();
  void ComputeNormal();
  float Normal[3];
  float SRange[2];
  float TRange[2];
  int AutomaticNormalGeneration;
};

#endif


