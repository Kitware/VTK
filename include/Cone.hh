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
// vlCone computes the implicit function and function gradient for a cone.
// vlCone is a concrete implementation of vlImplicitFunction. The cone is
// centered at the origin with axis of rotation coincident with z-axis.

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
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get height of cone.
  vlSetMacro(Height,float);
  vlGetMacro(Height,float);

  // Description:
  // Set/Get the radius of the cone's base.
  vlSetMacro(BaseRadius,float);
  vlGetMacro(BaseRadius,float);

protected:
  float Height;
  float BaseRadius;

};

#endif


