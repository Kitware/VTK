/*=========================================================================

  Program:   Visualization Library
  Module:    UserDef.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UserDef.hh"

vlUserDefined *vlUserDefined::MakeObject(int sze, int ext)
{
  return new vlUserDefined(sze,ext);
}

// Description:
// Deep copy of UserDefined data.
vlUserDefined& vlUserDefined::operator=(const vlUserDefined& ud)
{
  this->UD = ud.UD;
  return *this;
}

// Description:
// Given a list of pt ids, return an array of point coordinates.
void vlUserDefined::GetUserDefined(vlIdList& ptId, vlUserDefined& ud)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ud.InsertUserDefined(i,this->GetUserDefined(ptId.GetId(i)));
    }
}

void vlUserDefined::PrintSelf(ostream& os, vlIndent indent)
{
  vlRefCount::PrintSelf(os,indent);

  os << indent << "Number Of User Defined: " << this->GetNumberOfUserDefined() << "\n";
}

