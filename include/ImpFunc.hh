/*=========================================================================

  Program:   Visualization Library
  Module:    ImpFunc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlImplicitFunction - abstract interface for implicit functions
// .SECTION Description
// vlImplicitFunction specifies an abstract interface for implicit 
// functions. Implicit functions are of the form F(x,y,z) = 0.

#ifndef __vlImplicitFunction_h
#define __vlImplicitFunction_h

#include "Object.hh"

class vlImplicitFunction : public vlObject
{
public:
  char *GetClassName() {return "vlImplicitFunction";};

  // Description:
  // Evaluate function at position x-y-z and return value.
  virtual float Evaluate(float x, float y, float z) = 0;

  // Description:
  // Evaluate function gradient at position x-y-z and pass back vector.
  virtual void EvaluateGradient(float x, float y, float z, float g[3]) = 0;

};

#endif
