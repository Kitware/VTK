/*=========================================================================

  Program:   Visualization Library
  Module:    Sphere.hh
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
// Object for computation on sphere
//
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

  vlSetMacro(Radius,float);
  vlGetMacro(Radius,float);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float);

protected:
  float Radius;
  float Origin[3];

};

#endif


