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
// .NAME vlSphere - implicit function for a sphere
// .SECTION Description
// vlSphere computes the implicit function and/or gradient for a sphere.
// vlSphere is a concrete implementation of vlImplicitFunction.

#ifndef __vlSphere_h
#define __vlSphere_h

#include "ImpFunc.hh"

class vlSphere : public vlImplicitFunction
{
public:
  vlSphere();
  char *GetClassName() {return "vlSphere";};
  void PrintSelf(ostream& os, vlIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  vlSetMacro(Radius,float);
  vlGetMacro(Radius,float);

  vlSetVector3Macro(Center,float);
  vlGetVectorMacro(Center,float,3);

protected:
  float Radius;
  float Center[3];

};

#endif


