/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UserDef.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UserDef.hh"

vtkUserDefined *vtkUserDefined::MakeObject(int sze, int ext)
{
  return new vtkUserDefined(sze,ext);
}

// Description:
// Deep copy of UserDefined data.
vtkUserDefined& vtkUserDefined::operator=(const vtkUserDefined& ud)
{
  this->UD = ud.UD;
  return *this;
}

// Description:
// Given a list of pt ids, return an array of point coordinates.
void vtkUserDefined::GetUserDefined(vtkIdList& ptId, vtkUserDefined& ud)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ud.InsertUserDefined(i,this->GetUserDefined(ptId.GetId(i)));
    }
}

void vtkUserDefined::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of User Defined: " << this->GetNumberOfUserDefined() << "\n";
}

