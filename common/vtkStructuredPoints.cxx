/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkStructuredPoints.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkVoxel.h"

vtkStructuredPoints::vtkStructuredPoints()
{
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;
  
  this->AspectRatio[0] = 1.0;
  this->AspectRatio[1] = 1.0;
  this->AspectRatio[2] = 1.0;

  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;
}

vtkStructuredPoints::vtkStructuredPoints(const vtkStructuredPoints& v) :
vtkDataSet(v)
{
  this->Dimensions[0] = v.Dimensions[0];
  this->Dimensions[1] = v.Dimensions[1];
  this->Dimensions[2] = v.Dimensions[2];
  this->DataDescription = v.DataDescription;
  
  this->AspectRatio[0] = v.AspectRatio[0];
  this->AspectRatio[1] = v.AspectRatio[1];
  this->AspectRatio[2] = v.AspectRatio[2];

  this->Origin[0] = v.Origin[0];
  this->Origin[1] = v.Origin[1];
  this->Origin[2] = v.Origin[2];
}

vtkStructuredPoints::~vtkStructuredPoints()
{
}

// Description:
// Copy the geometric and topological structure of an input structured points 
// object.
void vtkStructuredPoints::CopyStructure(vtkDataSet *ds)
{
  vtkStructuredPoints *sPts=(vtkStructuredPoints *)ds;
  this->Initialize();

  for (int i=0; i<3; i++)
    {
    this->Dimensions[i] = sPts->Dimensions[i];
    this->Origin[i] = sPts->Origin[i];
    this->AspectRatio[i] = sPts->AspectRatio[i];
    }
 }

vtkCell *vtkStructuredPoints::GetCell(int cellId)
{
  static vtkVertex vertex;
  static vtkLine line;
  static vtkPixel pixel;
  static vtkVoxel voxel;
  static vtkCell *cell;
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  // 
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = &vertex;
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      cell = &line;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      cell = &line;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      cell = &line;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      cell = &pixel;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      cell = &pixel;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      cell = &pixel;
      break;

    case VTK_XYZ_GRID:
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
        cell->PointIds.SetId(npts,idx);
        cell->Points.SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

float *vtkStructuredPoints::GetPoint(int ptId)
{
  static float x[3];
  int i, loc[3];
  
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: 
      loc[0] = loc[1] = loc[2] = 0;
      break;

    case VTK_X_LINE:
      loc[1] = loc[2] = 0;
      loc[0] = ptId;
      break;

    case VTK_Y_LINE:
      loc[0] = loc[2] = 0;
      loc[1] = ptId;
      break;

    case VTK_Z_LINE:
      loc[0] = loc[1] = 0;
      loc[2] = ptId;
      break;

    case VTK_XY_PLANE:
      loc[2] = 0;
      loc[0] = ptId % this->Dimensions[0];
      loc[1] = ptId / this->Dimensions[0];
      break;

    case VTK_YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % this->Dimensions[1];
      loc[2] = ptId / this->Dimensions[1];
      break;

    case VTK_XZ_PLANE:
      loc[1] = 0;
      loc[0] = ptId % this->Dimensions[0];
      loc[2] = ptId / this->Dimensions[0];
      break;

    case VTK_XYZ_GRID:
      loc[0] = ptId % this->Dimensions[0];
      loc[1] = (ptId / this->Dimensions[0]) % this->Dimensions[1];
      loc[2] = ptId / (this->Dimensions[0]*this->Dimensions[1]);
      break;
    }

  for (i=0; i<3; i++)
    x[i] = this->Origin[i] + loc[i] * this->AspectRatio[i];

  return x;
}

int vtkStructuredPoints::FindPoint(float x[3])
{
  int i, loc[3];
  float d;
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
      loc[i] = (int) ((d / this->AspectRatio[i]) + 0.5);
      }
    }
//
//  From this location get the point id
//
  return loc[2]*this->Dimensions[0]*this->Dimensions[1] +
         loc[1]*this->Dimensions[0] + loc[0];
  
}

int vtkStructuredPoints::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                  float vtkNotUsed(tol2), int& subId, 
                                  float pcoords[3], float *weights)
{
  int loc[3];

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return -1;
    }

  vtkVoxel::InterpolationFunctions(pcoords,weights);
//
//  From this location get the cell id
//
  subId = 0;
  return loc[2] * (this->Dimensions[0]-1)*(this->Dimensions[1]-1) +
         loc[1] * (this->Dimensions[0]-1) + loc[0];
}

vtkCell *vtkStructuredPoints::FindAndGetCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                  float vtkNotUsed(tol2), int& subId, 
                                  float pcoords[3], float *weights)
{
  int i, j, k, loc[3];
  int npts, idx;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float xOut[3];
  static vtkCell *cell;
  static vtkVoxel voxel;

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return NULL;
    }
//
// Get the parametric coordinates and weights for interpolation
//
  vtkVoxel::InterpolationFunctions(pcoords,weights);
//
// Build a voxel
//
  cell = &voxel;
  for (npts=0,k = loc[2]; k <= loc[2] + 1; k++)
    {
    xOut[2] = this->Origin[2] + k * this->AspectRatio[2]; 
    for (j = loc[1]; j <= loc[1] + 1; j++)
      {
      xOut[1] = this->Origin[1] + j * this->AspectRatio[1]; 
      idx = loc[0] + j*this->Dimensions[0] + k*d01;
      for (i = loc[0]; i <= loc[0] + 1; i++, idx++)
        {
        xOut[0] = this->Origin[0] + i * this->AspectRatio[0]; 

        cell->PointIds.SetId(npts,idx);
        cell->Points.SetPoint(npts++,xOut);
        }
      }
    }

  subId = 0;
  return cell;
}

int vtkStructuredPoints::GetCellType(int vtkNotUsed(cellId))
{
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: 
      return VTK_VERTEX;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE:
      return VTK_LINE;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE:
      return VTK_PIXEL;

    case VTK_XYZ_GRID:
      return VTK_VOXEL;

    default:
      vtkErrorMacro(<<"Bad data description!");
      return VTK_NULL_ELEMENT;
    }
}

void vtkStructuredPoints::ComputeBounds()
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
void vtkStructuredPoints::GetVoxelGradient(int i, int j, int k, vtkScalars *s, 
                                          vtkFloatVectors& g)
{
  float gv[3];
  int ii, jj, kk, idx=0;

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
void vtkStructuredPoints::GetPointGradient(int i,int j,int k, vtkScalars *s, 
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
  if ( dims[2] == 1 )
    {
    g[2] = 0.0;
    }
  else if ( k == 0 )
    {
    sp = s->GetScalar(i + j*dims[0] + (k+1)*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[2] = (sp - sm) / ar[2];
    }
  else if ( k == (dims[2]-1) )
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

// Description:
// Set dimensions of structured points dataset.
void vtkStructuredPoints::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;
  this->SetDimensions(dim);
}

// Description:
// Set dimensions of structured points dataset.
void vtkStructuredPoints::SetDimensions(int dim[3])
{
  int returnStatus=vtkStructuredData::SetDimensions(dim,this->Dimensions);

  if ( returnStatus > -1 ) 
    {
    this->DataDescription = returnStatus;
    this->Modified();
    }
  else
   {
   vtkErrorMacro (<< "Bad Dimensions, retaining previous values");
   }
}

// Description:
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkStructuredPoints::ComputeStructuredCoordinates(float x[3], int ijk[3], 
                                                      float pcoords[3])
{
  int i;
  float d, floatLoc[3];
//
//  Compute the ijk location
//
  for (i=0; i<3; i++) 
    {
    d = x[i] - this->Origin[i];
    if ( d < 0.0 || d > ((this->Dimensions[i]-1)*this->AspectRatio[i]) ) 
      {
      return 0;
      } 
    else 
      {
      floatLoc[i] = d / this->AspectRatio[i];
      ijk[i] = (int) floatLoc[i];
      pcoords[i] = floatLoc[i] - (float)ijk[i];
      }
    }
  return 1;
}

void vtkStructuredPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "AspectRatio: (" << this->AspectRatio[0] << ", "
                                  << this->AspectRatio[1] << ", "
                                  << this->AspectRatio[2] << ")\n";

  os << indent << "Origin: (" << this->Origin[0] << ", "
                                  << this->Origin[1] << ", "
                                  << this->Origin[2] << ")\n";
}

