/*=========================================================================

  Program:   Visualization Library
  Module:    BScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
//
//  Scalar values, floating representation
// 
#include "BScalars.hh"

vlScalars *vlBitScalars::MakeObject(int sze, int ext)
{
  return new vlBitScalars(sze,ext);
}

vlBitScalars& vlBitScalars::operator=(const vlBitScalars& cs)
{
  this->S = cs.S;
  return *this;
}
