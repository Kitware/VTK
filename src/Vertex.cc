/*=========================================================================

  Program:   Visualization Library
  Module:    Vertex.cc
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
#include <math.h>
#include "Point.hh"

float vlPoint::DistanceToPoint (float *x)
{
  int numPts;
  float *X;

  if ( !this->Points || (numPts=this->Points->NumberOfPoints()) < 1 )
    return LARGE_FLOAT;

  X = this->Points->GetPoint(0);

  return (float)sqrt((X[0]-x[0])*(X[0]-x[0]) +
                     (X[1]-x[1])*(X[1]-x[1]) +
                     (X[2]-x[2])*(X[2]-x[2]));
}

