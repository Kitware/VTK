/*=========================================================================

  Program:   Visualization Library
  Module:    ImpFunc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ImpFunc.hh"

vlImplicitFunction::vlImplicitFunction()
{
  this->Transform = NULL;
}

// Description:
// Evaluate function at position x-y-z and return value. Point x[3] is
// transformed through transform (if provided).
float vlImplicitFunction::FunctionValue(float x[3])
{
  if ( ! this->Transform )
    {
    return this->EvaluateFunction(x);
    }

  else //pass point through transform
    {
    float pt[4];
    int i;

    pt[0] = x[0];
    pt[1] = x[1];
    pt[2] = x[2];
    pt[3] = 1.0;
    this->Transform->PointMultiply(pt,pt);
    if (pt[3] != 1.0 ) for (i=0; i<3; i++) pt[i] /= pt[3];

    return this->EvaluateFunction(pt);
    }
}

// Description:
// Evaluate function gradient at position x-y-z and pass back vector. Point
// x[3] is transformed through transform (if provided).
void vlImplicitFunction::FunctionGradient(float x[3], float g[3])
{
  if ( ! this->Transform )
    {
    this->EvaluateGradient(x,g);
    }

  else //pass point through transform
    {
    }
}

void vlImplicitFunction::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    os << indent << "Transform: (None)\n";
}

