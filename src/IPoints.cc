/*=========================================================================

  Program:   Visualization Library
  Module:    IPoints.cc
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
//  3D points, integer representation
//
//
#include "IPoints.hh"

vlPoints *vlIntPoints::MakeObject(int sze, int ext)
{
  return new vlIntPoints(sze,ext);
}

vlIntPoints& vlIntPoints::operator=(const vlIntPoints& fp)
{
  this->P = fp.P;
  return *this;
}

