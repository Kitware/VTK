/*=========================================================================

  Program:   Visualization Library
  Module:    Cone.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cone.hh"

// Description
// Construct cone with apex at (0,0,1), base at (0,0,0) and base radius=0.5.
vlCone::vlCone()
{
  this->Apex[0] = 0.0;
  this->Apex[1] = 0.0;
  this->Apex[2] = 0.0;

  this->Base[0] = 0.0;
  this->Base[1] = 0.0;
  this->Base[2] = 0.0;

  this->BaseRadius = 0.5;
}

// Description
// Evaluate cone equation.
float vlCone::Evaluate(float x, float y, float z)
{
  return 0;
}

// Description
// Evaluate cone normal.
void vlCone::EvaluateNormal(float x, float y, float z, float n[3])
{
}

void vlCone::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Apex: (" << this->Apex[0] << ", " 
    << this->Apex[1] << ", " << this->Apex[2] << ")\n";
  os << indent << "Base: (" << this->Base[0] << ", " 
    << this->Base[1] << ", " << this->Base[2] << ")\n";
  os << indent << "BaseRadius: " << this->BaseRadius << "\n";
}
