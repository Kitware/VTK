/*=========================================================================

  Program:   Visualization Library
  Module:    PtPicker.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtPicker.hh"

vlPointPicker::vlPointPicker()
{
}

void vlPointPicker::Intersect(float p1[3], float p2[3], float tol, 
                             vlActor *a, vlMapper *m)
{
}


void vlPointPicker::Initialize()
{
}

void vlPointPicker::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlPicker::PrintSelf(os,indent);
}
