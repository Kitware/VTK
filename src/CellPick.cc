/*=========================================================================

  Program:   Visualization Library
  Module:    CellPick.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CellPick.hh"

vlCellPicker::vlCellPicker()
{
}

void vlCellPicker::Intersect(float p1[3], float p2[3], float tol, 
                             vlActor *a, vlMapper *m)
{
}


void vlCellPicker::Initialize()
{
}

void vlCellPicker::PrintSelf(ostream& os, vlIndent indent)
{
  this->vlPicker::PrintSelf(os,indent);
}
