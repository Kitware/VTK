/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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

unsigned long vlStructuredPoints::GetMtime()
{
  unsigned long dtime = this->vlDataSet::GetMTime();
  unsigned long ftime = this->vlStructuredData::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredPoints::Initialize()
{
  vlStructuredData::_Initialize();

  this->SetAspectRatio(1,1,1);
  this->SetOrigin(0,0,0);
}

int vlStructuredPoints::FindCell(float x[3], vlCell *cell, float tol2, 
                                 int& subId, float pcoords[3],
                                 float weights[MAX_CELL_SIZE])
{
  int i, loc[3];
  float d, floatLoc[3];
  static vlVoxel voxel;
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
  voxel.InterpolationFunctions(pcoords,weights);
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

void vlStructuredPoints::ComputeBounds()
{
  this->Bounds[0] = this->Origin[0];
  this->Bounds[2] = this->Origin[1];
  this->Bounds[4] = this->Origin[2];

  this->Bounds[1] = this->Origin[0] + 
                    (this->Dimensions[0]-1) * this->AspectRatio[0];
  this->Bounds[3] = this->Origin[1] + 
                    (this->Dimensions[1]-1) * this->AspectRatio[1];
  this->Bounds[5] = this->Origin[2] +
                    (this->Dimensions[2]-1) * this->AspectRatio[2];
}

// Description:
// Given structured coordinates (i,j,k) for a voxel cell, compute the eight 
// gradient values for the voxel corners. The order in which the gradient
// vectors are arranged corresponds to the ordering of the voxel points. 
// Gradient vector is computed by central differences (except on edges of 
// volume where forward difference is used). The scalars s are the scalars
// from which the gradient is to be computed. This method will treat 
// only 3D structured point datasets (i.e., volumes).
void vlStructuredPoints::GetVoxelGradient(int i, int j, int k, vlScalars *s, 
                                          vlFloatVectors& g)
{
  float gv[3];
  int ii, jj, kk, idx;

  for ( kk=0; kk < 2; kk++)
    {
    for ( jj=0; jj < 2; jj++)
      {
      for ( ii=0; ii < 2; ii++)
        {
        this->GetPointGradient(i+ii, j+jj, k+kk, s, gv);
        g.SetVector(idx++, gv);
        }
      } 
    }
}

// Description:
// Given structured coordinates (i,j,k) for a point in a structured point 
// dataset, compute the gradient vector from the scalar data at that point. 
// The scalars s are the scalars from which the gradient is to be computed.
// This method will treat structured point datasets of any dimension.
void vlStructuredPoints::GetPointGradient(int i,int j,int k, vlScalars *s, 
                                          float g[3])
{
  int *dims=this->Dimensions;
  float *ar=this->AspectRatio;
  int ijsize=dims[0]*dims[1];
  float sp, sm;

  // x-direction
  if ( dims[0] == 1 )
    {
    g[0] = 0.0;
    }
  else if ( i == 0 )
    {
    sp = s->GetScalar(i+1 + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[0] = (sp - sm) / ar[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i-1 + j*dims[0] + k*ijsize);
    g[0] = (sp - sm) / ar[0];
    }
  else
    {
    sp = s->GetScalar(i+1 + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i-1 + j*dims[0] + k*ijsize);
    g[0] = 0.5 * (sp - sm) / ar[0];
    }

  // y-direction
  if ( dims[1] == 1 )
    {
    g[1] = 0.0;
    }
  else if ( j == 0 )
    {
    sp = s->GetScalar(i + (j+1)*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[1] = (sp - sm) / ar[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + (j-1)*dims[0] + k*ijsize);
    g[1] = (sp - sm) / ar[1];
    }
  else
    {
    sp = s->GetScalar(i + (j+1)*dims[0] + k*ijsize);
    sm = s->GetScalar(i + (j-1)*dims[0] + k*ijsize);
    g[1] = 0.5 * (sp - sm) / ar[1];
    }

  // z-direction
  if ( dims[1] == 1 )
    {
    g[2] = 0.0;
    }
  else if ( k == 0 )
    {
    sp = s->GetScalar(i + j*dims[0] + (k+1)*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[2] = (sp - sm) / ar[2];
    }
  else if ( k == (dims[1]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + (k-1)*ijsize);
    g[2] = (sp - sm) / ar[2];
    }
  else
    {
    sp = s->GetScalar(i + j*dims[0] + (k+1)*ijsize);
    sm = s->GetScalar(i + j*dims[0] + (k-1)*ijsize);
    g[2] = 0.5 * (sp - sm) / ar[2];
    }
}

void vlStructuredPoints::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSet::PrintSelf(os,indent);

  os << indent << "Origin: (" << this->Origin[0] << ", "
                                  << this->Origin[1] << ", "
                                  << this->Origin[2] << ")\n";
  os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                  << this->AspectRatio[1] << ", "
                                  << this->AspectRatio[2] << ")\n";
}

