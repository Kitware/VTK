/*=========================================================================

  Program:   Visualization Library
  Module:    IScalars.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "IScalars.hh"

vlScalars *vlIntScalars::MakeObject(int sze, int ext)
{
  return new vlIntScalars(sze,ext);
}

// Description:
// Deep copy of scalars.
vlIntScalars& vlIntScalars::operator=(const vlIntScalars& is)
{
  this->S = is.S;
  return *this;
}

void vlIntScalars::GetScalars(vlIdList& ptId, vlFloatScalars& fs)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fs.InsertScalar(i,(float)this->S.GetValue(ptId.GetId(i)));
    }
}
