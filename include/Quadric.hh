/*=========================================================================

  Program:   Visualization Library
  Module:    Quadric.hh
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
// Quadric function a0*x^2 + a1*y^2 + a2*z^2 + a3*x*y + a4*y*z + a5*x*z +
//                  a6*x + a7*y + a8*z + a9
//
#ifndef __vlQuadric_h
#define __vlQuadric_h

#include "ImpFunc.hh"

class vlQuadric : public vlImplicitFunction
{
public:
  vlQuadric();
  char *GetClassName() {return "vlQuadric";};

  // ImplicitFunction interface
  float Evaluate(float x, float y, float z);

  void SetCoefficients(float a[10]);
  void SetCoefficients(float a0, float a1, float a2, float a3, float a4, 
                       float a5, float a6, float a7, float a8, float a9);
  vlGetVectorMacro(Coefficients,float);

protected:
  float Coefficients[10];

};

#endif


