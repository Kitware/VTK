/*=========================================================================

  Program:   Visualization Library
  Module:    Cone.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCone - implicit function for a cone
// .SECTION Description
// vlCone computes the implicit function and surface normal for a cone.
// vlCone is a concrete implementation of vlImplicitFunction.

#ifndef __vlCone_h
#define __vlCone_h

#include "ImpFunc.hh"

class vlCone : public vlImplicitFunction
{
public:
  vlCone();
  char *GetClassName() {return "vlCone";};
  void PrintSelf(ostream& os, vlIndent indent);

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);
  void EvaluateNormal(float x, float y, float z, float n[3]);

  vlSetVector3Macro(Apex,float);
  vlGetVectorMacro(Apex,float,3);

  vlSetVector3Macro(Base,float);
  vlGetVectorMacro(Base,float,3);

  vlSetMacro(BaseRadius,float);
  vlGetMacro(BaseRadius,float);

protected:
  float Apex[3];
  float Base[3];
  float BaseRadius;

};

#endif


