/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TCoords.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TCoords.hh"
#include "IdList.hh"
#include "FTCoords.hh"

void vtkTCoords::GetTCoord(int id, float tc[3])
{
  float *tcp = this->GetTCoord(id);
  for (int i=0; i<this->Dimension; i++) tc[i] = tcp[i];
}

// Description:
// Insert texture coordinate into position indicated. Although up to three
// texture components may be specified (i.e., tc1, tc2, tc3), if the texture
// coordinates are less than 3 dimensions the extra components will be ignored.
void vtkTCoords::InsertTCoord(int id, float tc1, float tc2, float tc3)
{
  float tc[3];

  tc[0] = tc1;
  tc[1] = tc2;
  tc[2] = tc3;
  this->InsertTCoord(id,tc);
}

// Description:
// Insert texture coordinate into position indicated. Although up to three
// texture components may be specified (i.e., tc1, tc2, tc3), if the texture
// coordinates are less than 3 dimensions the extra components will be ignored.
int vtkTCoords::InsertNextTCoord(float tc1, float tc2, float tc3)
{
  float tc[3];

  tc[0] = tc1;
  tc[1] = tc2;
  tc[2] = tc3;
  return this->InsertNextTCoord(tc);
}

// Description:
// Construct object whose texture coordinates are of specified dimension.
vtkTCoords::vtkTCoords(int dim)
{
  this->Dimension = dim;
}

// Description:
// Given a list of pt ids, return an array of texture coordinates.
void vtkTCoords::GetTCoords(vtkIdList& ptId, vtkFloatTCoords& ftc)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ftc.InsertTCoord(i,this->GetTCoord(ptId.GetId(i)));
    }
}

void vtkTCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Texture Coordinates: " << this->GetNumberOfTCoords() << "\n";
  os << indent << "Texture Dimension: " << this->Dimension << "\n";
}
