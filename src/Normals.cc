/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Normals.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Normals.hh"
#include "IdList.hh"
#include "FNormals.hh"

void vtkNormals::GetNormal(int id, float n[3])
{
  float *np = this->GetNormal(id);
  for (int i=0; i<3; i++) n[i] = np[i];
}

// Description:
// Insert normal into position indicated.
void vtkNormals::InsertNormal(int id, float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  this->InsertNormal(id,n);
}

// Description:
// Insert normal into position indicated.
int vtkNormals::InsertNextNormal(float nx, float ny, float nz)
{
  float n[3];

  n[0] = nx;
  n[1] = ny;
  n[2] = nz;
  return this->InsertNextNormal(n);
}

// Description:
// Given a list of pt ids, return an array of corresponding normals.
void vtkNormals::GetNormals(vtkIdList& ptId, vtkFloatNormals& fp)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    fp.InsertNormal(i,this->GetNormal(ptId.GetId(i)));
    }
}

void vtkNormals::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Normals: " << this->GetNumberOfNormals() << "\n";
}
