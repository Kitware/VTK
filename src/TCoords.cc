/*=========================================================================

  Program:   Visualization Library
  Module:    TCoords.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "TCoords.hh"
#include "IdList.hh"
#include "FTCoords.hh"

// Description:
// Construct object whose texture coordinates are of specified dimension.
vlTCoords::vlTCoords(int dim)
{
  this->Dimension = dim;
}

// Description:
// Given a list of pt ids, return an array of texture coordinates.
void vlTCoords::GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ftc.InsertTCoord(i,this->GetTCoord(ptId.GetId(i)));
    }
}

void vlTCoords::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlTCoords::GetClassName()))
    {
    vlRefCount::PrintSelf(os,indent);

    os << indent << "Number Of Texture Coordinates: " << this->GetNumberOfTCoords() << "\n";
    os << indent << "Texture Dimension: " << this->Dimension << "\n";
    }
}
