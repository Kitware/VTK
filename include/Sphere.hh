/*=========================================================================

  Program:   Visualization Library
  Module:    Sphere.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSphere - implicit function of a sphere
// .SECTION Description
// vlSphere computes the implicit function and surface normal for a sphere.
// vlSphere is a concrete implementation of vlImplicitFunction.

#ifndef __vlSphere_h
#define __vlSphere_h

#include "ImpFunc.hh"

class vlSphere : public vlImplicitFunction
{
public:
  vlSphere();
  char *GetClassName() {return "vlSphere";};

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);
  void EvaluateNormal(float x, float y, float z, float n[3]);

  vlSetMacro(Radius,float);
  vlGetMacro(Radius,float);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float,3);

protected:
  float Radius;
  float Origin[3];

};

#endif


