/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.cc
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
#include "SPoints.hh"
#include "Point.hh"
#include "Line.hh"
#include "Rect.hh"
#include "Brick.hh"

vlStructuredPoints::vlStructuredPoints()
{
  this->Dimension[0] = 1;
  this->Dimension[1] = 1;
  this->Dimension[2] = 1;
  this->DataDescription = SINGLE_POINT;

  this->AspectRatio[0] = 1.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

vlStructuredPoints::vlStructuredPoints(const vlStructuredPoints& v)
{
  this->Dimension[0] = v.Dimension[0];
  this->Dimension[1] = v.Dimension[1];
  this->Dimension[2] = v.Dimension[2];
  this->DataDescription = v.DataDescription;

  this->AspectRatio[0] = v.AspectRatio[0];
  this->AspectRatio[1] = v.AspectRatio[1];
  this->AspectRatio[2] = v.AspectRatio[2];

  this->Origin[0] = v.Origin[0];
  this->Origin[1] = v.Origin[1];
  this->Origin[2] = v.Origin[2];
}

vlStructuredPoints::~vlStructuredPoints()
{
}

int vlStructuredPoints::NumberOfCells()
{
  int nCells=1;
  int i;

  for (i=0; i<3; i++)
    if (this->Dimension[i] > 1)
      nCells *= (this->Dimension[1]-1);

  return nCells;
}

int vlStructuredPoints::NumberOfPoints()
{
  return Dimension[0]*Dimension[1]*Dimension[2];
}


vlCell *vlStructuredPoints::GetCell(int cellId)
{
  static vlPoint point;
  static vlLine line;
  static vlRectangle rectangle;
  static vlBrick brick;
  static vlCell *cell;
  int i, j, k, idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimension[0]*this->Dimension[1];
  float x[3];
 
  // 
  switch (this->DataDescription)
    {
    case SINGLE_POINT: // cellId can only be = 0
      iMin = iMax = jMin = jMax = kMin = kMax = 0;
      cell = &point;
      break;

    case X_LINE:
      jMin = jMax = kMin = kMax = 0;
      iMin = cellId;
      iMax = cellId + 1;
      cell = &line;
      break;

    case Y_LINE:
      iMin = iMax = kMin = kMax = 0;
      jMin = cellId;
      jMax = cellId + 1;
      cell = &line;
      break;

    case Z_LINE:
      iMin = iMax = jMin = jMax = 0;
      kMin = cellId;
      kMax = cellId + 1;
      cell = &line;
      break;

    case XY_PLANE:
      kMin = kMax = 0;
      iMin = cellId % (this->Dimension[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimension[0]-1);
      jMax = jMin + 1;
      cell = &rectangle;
      break;

    case YZ_PLANE:
      iMin = iMax = 0;
      jMin = cellId % (this->Dimension[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimension[1]-1);
      kMax = kMin + 1;
      cell = &rectangle;
      break;

    case XZ_PLANE:
      jMin = jMax = 0;
      iMin = cellId % (this->Dimension[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimension[0]-1);
      kMax = kMin + 1;
      cell = &rectangle;
      break;

    case XYZ_GRID:
      int cd01 = this->Dimension[0]*this->Dimension[1];
      iMin = cellId % (this->Dimension[0]-1);
      iMax = iMin + 1;
      jMin = (cellId % cd01) / (this->Dimension[0]-1);
      jMax = jMin + 1;
      kMin = cellId / cd01;
      kMax = kMin + 1;
      cell = &brick;
      break;
    }

  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = this->Origin[2] + loc[2] * this->AspectRatio[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = this->Origin[1] + loc[1] * this->AspectRatio[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = this->Origin[0] + loc[0] * this->AspectRatio[0]; 
        idx = loc[0] + loc[1]*this->Dimension[0] + loc[2]*d01;
        cell->PointIds.InsertId(npts,idx);
        cell->Points.SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

float *vlStructuredPoints::GetPoint(int ptId)
{
  static float x[3];
  int i, loc[3];
  
  switch (this->DataDescription)
    {
    case SINGLE_POINT: 
      loc[0] = loc[1] = loc[2] = 0;
      break;

    case X_LINE:
      loc[1] = loc[2] = 0;
      loc[0] = ptId;
      break;

    case Y_LINE:
      loc[0] = loc[2] = 0;
      loc[1] = ptId;
      break;

    case Z_LINE:
      loc[0] = loc[1] = 0;
      loc[2] = ptId;
      break;

    case XY_PLANE:
      loc[2] = 0;
      loc[0] = ptId % this->Dimension[0];
      loc[1] = ptId / this->Dimension[0];
      break;

    case YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % this->Dimension[1];
      loc[2] = ptId / this->Dimension[1];
      break;

    case XZ_PLANE:
      loc[1] = 0;
      loc[0] = ptId % this->Dimension[0];
      loc[2] = ptId / this->Dimension[0];
      break;

    case XYZ_GRID:
      loc[0] = ptId % this->Dimension[0];
      loc[1] = (ptId % (this->Dimension[0]*this->Dimension[1]))
                    / this->Dimension[0];
      loc[2] = ptId / (this->Dimension[0]*this->Dimension[1]);
      break;
    }

  for (i=0; i<3; i++)
    x[i] = this->Origin[i] + loc[i] * this->AspectRatio[i];

  return x;
}

void vlStructuredPoints::SetDimension(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetDimension(dim);
}

void vlStructuredPoints::SetDimension(int dim[3])
{
  int i;

  if ( dim[0] != this->Dimension[0] || dim[1] != Dimension[1] ||
  dim[2] != Dimension[2] )
    {
    if ( dim[0]<1 || dim[1]<1 || dim[2]<1 )
      {
      vlErrorMacro (<< "Bad dimensions, retaining previous values");
      return;
      }

    for (int dataDim=0, i=0; i<3 ; i++)
      {
      this->Dimension[i] = dim[i];
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

void vlStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredPoints::GetClassName()))
    {
    vlDataSet::PrintSelf(os,indent);
    
    os << indent << "Dimension: (" << this->Dimension[0] << ", "
                                    << this->Dimension[1] << ", "
                                    << this->Dimension[2] << ")\n";
    os << indent << "Origin: (" << this->Origin[0] << ", "
                                    << this->Origin[1] << ", "
                                    << this->Origin[2] << ")\n";
    os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                    << this->AspectRatio[1] << ", "
                                    << this->AspectRatio[2] << ")\n";
    }
}

