/*=========================================================================

  Program:   Visualization Library
  Module:    StrData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
  this->PointVisibility = NULL;
}

vlStructuredDataSet::vlStructuredDataSet(const vlStructuredDataSet& sds) :
vlDataSet(sds)
{
  this->Dimensions[0] = sds.Dimensions[0];
  this->Dimensions[1] = sds.Dimensions[1];
  this->Dimensions[2] = sds.Dimensions[2];
  this->DataDescription = sds.DataDescription;

  this->Blanking = sds.Blanking;
  if ( sds.PointVisibility != NULL )
    this->PointVisibility = new vlBitArray(*sds.PointVisibility);
  else
    this->PointVisibility = NULL;
}

vlStructuredDataSet::~vlStructuredDataSet()
{
  this->Initialize();
}

// Description:
// Return the topological dimension of the data (e.g.,0, 1, 2, or 3D).
int vlStructuredDataSet::GetDataDimension()
{
  switch (this->DataDescription)
    {
    case SINGLE_POINT: return 0;

    case X_LINE: case Y_LINE: case Z_LINE: return 1;

    case XY_PLANE: case YZ_PLANE: case XZ_PLANE: return 2;

    case XYZ_GRID: return 3;
    }
}

// Description:
// Set the i-j-k dimensions of the data.
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
  vlDebugMacro(<< " setting Dimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

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
      nCells *= (this->Dimensions[i]-1);

  return nCells;
}

int vlStructuredDataSet::GetNumberOfPoints()
{
  return Dimensions[0]*Dimensions[1]*Dimensions[2];
}

// Description:
// Turn on data blanking. Data blanking is the ability to turn off
// portions of the grid when displaying or operating on it. Some data
// (like finite difference data) routinely turns off data to simulate
// solid obstacles.
void vlStructuredDataSet::BlankingOn()
{
  this->Blanking = 1;
  this->Modified();

  if ( !this->PointVisibility )
    {
    this->PointVisibility = new vlBitArray(this->GetNumberOfPoints(),1000);
    for (int i=0; i<this->GetNumberOfPoints(); i++)
      {
      this->PointVisibility->InsertValue(i,1);
      }
    }
}

// Description:
// Turn off data blanking.
void vlStructuredDataSet::BlankingOff()
{
  this->Blanking = 0;
  this->Modified();
}

// Description:
// Turn off a particular data point.
void vlStructuredDataSet::BlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->BlankingOn();
  this->PointVisibility->InsertValue(ptId,0);
}

// Description:
// Turn on a particular data point.
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
    delete this->PointVisibility;
    this->PointVisibility = NULL;
    }
}

void vlStructuredDataSet::GetCellPoints(int cellId, vlIdList& ptIds)
{
  int i, j, k, idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
 
  ptIds.Reset();

  switch (this->DataDescription)
    {
    case SINGLE_POINT: // cellId can only be = 0
      iMin = iMax = jMin = jMax = kMin = kMax = 0;
      break;

    case X_LINE:
      jMin = jMax = kMin = kMax = 0;
      iMin = cellId;
      iMax = cellId + 1;
      break;

    case Y_LINE:
      iMin = iMax = kMin = kMax = 0;
      jMin = cellId;
      jMax = cellId + 1;
      break;

    case Z_LINE:
      iMin = iMax = jMin = jMax = 0;
      kMin = cellId;
      kMax = cellId + 1;
      break;

    case XY_PLANE:
      kMin = kMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      break;

    case YZ_PLANE:
      iMin = iMax = 0;
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      break;

    case XZ_PLANE:
      jMin = jMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      break;

    case XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      break;
    }

  // Extract point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        ptIds.InsertId(npts++,idx);
        }
      }
    }
}

void vlStructuredDataSet::GetPointCells(int ptId, vlIdList& cellIds)
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
  ptLoc[1] = (ptId / ptDim[0]) % ptDim[1];
  ptLoc[2] = ptId / (ptDim[0]*ptDim[1]);
//
//  From the point lcoation, compute the cell locations.  There are at
//  most eight possible.
//
  cellIds.Reset();

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
      cellIds.InsertNextId(cellId);
      }
    }

  return;
}
