/*=========================================================================

  Program:   Visualization Library
  Module:    Sphere.cc
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
#include "Sphere.hh"

vlSphere::vlSphere()
{
  this->Radius = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

float vlSphere::Evaluate(float x, float y, float z)
{
  return ( sqrt ( this->Radius*this->Radius -
                ((x - this->Origin[0]) * (x - this->Origin[0]) + 
                 (y - this->Origin[1]) * (y - this->Origin[1]) + 
                 (z - this->Origin[2]) * (z - this->Origin[2])) ) );
}
