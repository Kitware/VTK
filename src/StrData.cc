/*=========================================================================

  Program:   Visualization Library
  Module:    StrData.cc
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
#include "StrData.hh"

vlStructuredDataSet::vlStructuredDataSet()
{
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = SINGLE_POINT;
}

vlStructuredDataSet::vlStructuredDataSet(const vlStructuredDataSet& sds)
{
  this->Dimensions[0] = sds.Dimensions[0];
  this->Dimensions[1] = sds.Dimensions[1];
  this->Dimensions[2] = sds.Dimensions[2];
  this->DataDescription = sds.DataDescription;
}

vlStructuredDataSet::~vlStructuredDataSet()
{
}


void vlStructuredDataSet::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetDimensions(dim);
}

void vlStructuredDataSet::SetDimensions(int dim[3])
{
  int i;

  if ( dim[0] != this->Dimensions[0] || dim[1] != Dimensions[1] ||
  dim[2] != Dimensions[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vlErrorMacro (<< "Bad Dimensions, retaining previous values");
      return;
      }

    for (int dataDim=0, i=0; i<3 ; i++)
      {
      this->Dimensions[i] = dim[i];
      if (dim[i] > 1) dataDim++;
      }

    if ( dataDim == 3 )
      {
      this->DataDescription = XYZ_GRID;
      }
    else if ( dataDim == 2)
      {
      if ( dim[0] == 1 ) this->DataDescription = YZ_PLANE;
      else if ( dim[1] == 1 ) this->DataDescription = XZ_PLANE;
      else this->DataDescription = XY_PLANE;
      }
    else if ( dataDim == 1 )
      {
      if ( dim[0] != 1 ) this->DataDescription = X_LINE;
      else if ( dim[1] != 1 ) this->DataDescription = Y_LINE;
      else this->DataDescription = Z_LINE;
      }
    else
      {
      this->DataDescription = SINGLE_POINT;
      }

    this->Modified();
    }
}

void vlStructuredDataSet::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredDataSet::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    
    os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                    << this->Dimensions[1] << ", "
                                    << this->Dimensions[2] << ")\n";
    }
}

int vlStructuredDataSet::GetNumberOfCells()
{
  int nCells=1;
  int i;

  for (i=0; i<3; i++)
    if (this->Dimensions[i] > 1)
      nCells *= (this->Dimensions[1]-1);

  return nCells;
}

int vlStructuredDataSet::GetNumberOfPoints()
{
  return Dimensions[0]*Dimensions[1]*Dimensions[2];
}

