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
  
  this->Blanking = 0;
  this->PointVisibility = 0;
}

vlStructuredDataSet::vlStructuredDataSet(const vlStructuredDataSet& sds)
{
  this->Dimensions[0] = sds.Dimensions[0];
  this->Dimensions[1] = sds.Dimensions[1];
  this->Dimensions[2] = sds.Dimensions[2];
  this->DataDescription = sds.DataDescription;

  this->Blanking = sds.Blanking;
  this->PointVisibility = sds.PointVisibility;
}

vlStructuredDataSet::~vlStructuredDataSet()
{
  this->Initialize();
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

void vlStructuredDataSet::BlankingOn()
{
  this->Blanking = 1;
  this->Modified();

  if ( !this->PointVisibility )
    {
    this->PointVisibility = new vlCharArray(this->GetNumberOfPoints(),1000);
    for (int i=0; i<this->GetNumberOfPoints(); i++)
      {
      this->PointVisibility->InsertValue(i,1);
      }
    }
}

void vlStructuredDataSet::BlankingOff()
{
  this->Blanking = 0;
  this->Modified();
}

void vlStructuredDataSet::BlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->BlankingOn();
  this->PointVisibility->InsertValue(ptId,0);
}

void vlStructuredDataSet::UnBlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->BlankingOn();
  this->PointVisibility->InsertValue(ptId,1);
}

void vlStructuredDataSet::Initialize()
{
  vlDataSet::Initialize();

  this->SetDimensions(1,1,1);
  this->Blanking = 0;

  if ( this->PointVisibility )
    {
    this->PointVisibility->UnRegister((void *)this);
    this->PointVisibility = 0;
    }
}

void vlStructuredDataSet::GetPointCells(int ptId, vlIdList *cellIds)
{
  int ptDim[3], cellDim[3];
  int ptLoc[3], cellLoc[3];
  int i, j, cellId, add;
  static int offset[8][3] = {-1,0,0, -1,-1,0, -1,-1,-1, -1,0,-1,
                               0,0,0,  0,-1,0,  0,-1,-1,  0,0,-1};

  for (i=0; i<3; i++) 
    {
    ptDim[i] = this->Dimensions[i];
    cellDim[i] = ptDim[i] - 1;
    }
//
//  Get the location of the point
//
  ptLoc[0] = ptId % ptDim[0];
  ptLoc[1] = ptId % (ptDim[0]*ptDim[1]) / ptDim[0];
  ptLoc[2] = ptId / (ptDim[0]*ptDim[1]);
//
//  From the point lcoation, compute the cell locations.  There are at
//  most eight possible.
//
  cellIds->Reset();

  for (j=0; j<8; j++) 
    {
    for (add=1, i=0; i<3; i++) 
      {
      cellLoc[i] = ptLoc[i] + offset[j][i];
      if ( cellLoc[i] < 0 || cellLoc[i] >= cellDim[i] ) 
        {
        add = 0;
        break;
        }
      }
    if ( add ) 
      {
      cellId = cellLoc[0] + cellLoc[1]*cellDim[0] + 
                            cellLoc[2]*cellDim[0]*cellDim[1];
      cellIds->InsertNextId(cellId);
      }
    }

  return;
}
