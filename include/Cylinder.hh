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
// vlCylinder computes the implicit function and function gradient for 
// a cylinder. vlCylinder is a concrete implementation of vlImplicitFunction.
// Cylinder is centered at origin and axes of rotation is along z-axis. (Use a
// transform filter if necessary to reposition).
// .SECTION Caveats
// The cylinder is infinite in extent. To truncate the cylinder use the 
// vlImplicitBoolean in combination with clipping planes.


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
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get cylinder radius.
  vlSetMacro(Radius,float);
  vlGetMacro(Radius,float);

protected:
  float Radius;

};

#endif


