/*=========================================================================

  Program:   Visualization Library
  Module:    SScalars.cc
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
#include "SScalars.hh"

vlScalars *vlShortScalars::MakeObject(int sze, int ext)
{
  return new vlShortScalars(sze,ext);
}

vlShortScalars& vlShortScalars::operator=(const vlShortScalars& ss)
{
  this->S = ss.S;
  return *this;
}
