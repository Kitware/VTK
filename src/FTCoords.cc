/*=========================================================================

  Program:   Visualization Library
  Module:    FTCoords.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "FTCoords.hh"

vlTCoords *vlFloatTCoords::MakeObject(int sze, int d, int ext)
{
  return new vlFloatTCoords(sze,d,ext);
}

// Description:
// Deep copy of texture coordinates.
vlFloatTCoords& vlFloatTCoords::operator=(const vlFloatTCoords& ftc)
{
  this->TC = ftc.TC;
  this->Dimension = ftc.Dimension;
  
  return *this;
}

