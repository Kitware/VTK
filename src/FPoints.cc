/*=========================================================================

  Program:   Visualization Library
  Module:    FPoints.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FPoints.hh"

vlPoints *vlFloatPoints::MakeObject(int sze, int ext)
{
  return new vlFloatPoints(sze,ext);
}

// Description:
// Deep copy of points.
vlFloatPoints& vlFloatPoints::operator=(const vlFloatPoints& fp)
{
  this->P = fp.P;
  return *this;
}

