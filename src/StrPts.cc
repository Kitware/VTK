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
#include "StrPts.hh"
#include "Vertex.hh"
#include "Line.hh"
#include "Pixel.hh"
#include "Voxel.hh"

vlStructuredPoints::vlStructuredPoints()
{
  this->AspectRatio[0] = 1.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

vlStructuredPoints::vlStructuredPoints(const vlStructuredPoints& v) :
vlStructuredData(v)
{

  this->AspectRatio[1] = v.AspectRatio[1];
  this->AspectRatio[2] = v.AspectRatio[2];

  this->Origin[0] = v.Origin[0];
  this->Origin[1] = v.Origin[1];
  this->Origin[2] = v.Origin[2];
}

vlStructuredPoints::~vlStructuredPoints()
{
}

vlCell *vlStructuredPoints::GetCell(int cellId)
{
  static vlVertex vertex;
  static vlLine line;
  static vlPixel pixel;
  static vlVoxel voxel;
  static vlCell *cell;
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float x[3];
 
  // 
  switch (this->DataDescription)
    {
    case SINGLE_POINT: // cellId can only be = 0
      iMin = iMax = jMin = jMax = kMin = kMax = 0;
      cell = &vertex;
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
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      cell = &pixel;
      break;

    case YZ_PLANE:
      iMin = iMax = 0;
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      cell = &pixel;
      break;

    case XZ_PLANE:
      jMin = jMax = 0;
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      cell = &pixel;
      break;

    case XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      cell = &voxel;
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
        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        cell->PointIds.InsertId(npts,idx);
        cell->Points.InsertPoint(npts++,x);
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
      loc[0] = ptId % this->Dimensions[0];
      loc[1] = ptId / this->Dimensions[0];
      break;

    case YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % this->Dimensions[1];
      loc[2] = ptId / this->Dimensions[1];
      break;

    case XZ_PLANE:
      loc[1] = 0;
      loc[0] = ptId % this->Dimensions[0];
      loc[2] = ptId / this->Dimensions[0];
      break;

    case XYZ_GRID:
      loc[0] = ptId % this->Dimensions[0];
      loc[1] = (ptId / this->Dimensions[0]) % this->Dimensions[1];
      loc[2] = ptId / (this->Dimensions[0]*this->Dimensions[1]);
      break;
    }

  for (i=0; i<3; i++)
    x[i] = this->Origin[i] + loc[i] * this->AspectRatio[i];

  return x;
}

void vlStructuredPoints::Initialize()
{
  vlStructuredData::Initialize();

  this->SetAspectRatio(1,1,1);
  this->SetOrigin(0,0,0);
}

int vlStructuredPoints::FindCell(float x[3], vlCell *cell, float tol2, 
                                 int& subId, float pcoords[3])
{
  int i, loc[3];
  float d, floatLoc[3];
//
//  Compute the ijk location
//
  for (i=0; i<3; i++) 
    {
    d = x[i] - this->Origin[i];
    if ( d < 0.0 || d > ((this->Dimensions[i]-1)*this->AspectRatio[i]) ) 
      {
      return -1;
      } 
    else 
      {
      floatLoc[i] = d / this->AspectRatio[i];
      loc[i] = (int) floatLoc[i];
      pcoords[i] = floatLoc[i] - (float)loc[i];
      }
    }
//
//  From this location get the cell number
//
  subId = 0;
  return loc[2] * (this->Dimensions[0]-1)*(this->Dimensions[1]-1) +
         loc[1] * (this->Dimensions[0]-1) + loc[0];
}

int vlStructuredPoints::GetCellType(int cellId)
{
  switch (this->DataDescription)
    {
    case SINGLE_POINT: 
      return vlVERTEX;

    case X_LINE: case Y_LINE: case Z_LINE:
      return vlLINE;

    case XY_PLANE: case YZ_PLANE: case XZ_PLANE:
      return vlPIXEL;

    case XYZ_GRID:
      return vlVOXEL;

    default:
      vlErrorMacro(<<"Bad data description!");
      return vlNULL_ELEMENT;
    }
}

void vlStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredPoints::GetClassName()))
    {
    vlStructuredData::PrintSelf(os,indent);
    
    os << indent << "Origin: (" << this->Origin[0] << ", "
                                    << this->Origin[1] << ", "
                                    << this->Origin[2] << ")\n";
    os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                    << this->AspectRatio[1] << ", "
                                    << this->AspectRatio[2] << ")\n";
    }
}

