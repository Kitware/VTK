/*=========================================================================

  Program:   Visualization Library
  Module:    Cylinder.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCylinder - implicit function for a cylinder
// .SECTION Description
// vlCylinder computes the implicit function and surface normal for a cylinder.
// vlCylinder is a concrete implementation of vlImplicitFunction.

#ifndef __vlCylinder_h
#define __vlCylinder_h

#include "ImpFunc.hh"

class vlCylinder : public vlImplicitFunction
{
public:
  vlCylinder();
  char *GetClassName() {return "vlCylinder";};
  void PrintSelf(ostream& os, vlIndent indent);

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);
  void EvaluateNormal(float x, float y, float z, float n[3]);

  vlSetMacro(Radius,float);
  vlGetMacro(Radius,float);

  vlSetVector3Macro(Top,float);
  vlGetVectorMacro(Top,float,3);

  vlSetVector3Macro(Bottom,float);
  vlGetVectorMacro(Bottom,float,3);

protected:
  float Radius;
  float Top[3];
  float Bottom[3];

};

#endif


