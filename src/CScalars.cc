/*=========================================================================

  Program:   Visualization Library
  Module:    CScalars.cc
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
//  Scalar values, floating representation
// 
#include "CScalars.hh"

vlScalars *vlCharScalars::MakeObject(int sze, int ext)
{
  return new vlCharScalars(sze,ext);
}

vlCharScalars& vlCharScalars::operator=(const vlCharScalars& cs)
{
  this->S = cs.S;
  return *this;
}
