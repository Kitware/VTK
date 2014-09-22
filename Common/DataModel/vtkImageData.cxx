/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"

vtkStandardNewMacro(vtkImageData);

//----------------------------------------------------------------------------
vtkImageData::vtkImageData()
{
  int idx;

  this->Vertex = 0;
  this->Line = 0;
  this->Pixel = 0;
  this->Voxel = 0;

  this->DataDescription = VTK_EMPTY;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = 0;
    this->Increments[idx] = 0;
    this->Origin[idx] = 0.0;
    this->Spacing[idx] = 1.0;
    this->Point[idx] = 0.0;
    }

  int extent[6] = {0, -1, 0, -1, 0, -1};
  memcpy(this->Extent, extent, 6*sizeof(int));

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
}

//----------------------------------------------------------------------------
vtkImageData::~vtkImageData()
{
  if (this->Vertex)
    {
    this->Vertex->Delete();
    }
  if (this->Line)
    {
    this->Line->Delete();
    }
  if (this->Pixel)
    {
    this->Pixel->Delete();
    }
  if (this->Voxel)
    {
    this->Voxel->Delete();
    }
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points
// object.
void vtkImageData::CopyStructure(vtkDataSet *ds)
{
  vtkImageData *sPts=static_cast<vtkImageData *>(ds);
  this->Initialize();

  int i;
  for (i=0; i<3; i++)
    {
    this->Dimensions[i] = sPts->Dimensions[i];
    this->Spacing[i] = sPts->Spacing[i];
    this->Origin[i] = sPts->Origin[i];
    }
  this->SetExtent(sPts->GetExtent());
}

//----------------------------------------------------------------------------
void vtkImageData::Initialize()
{
  this->Superclass::Initialize();
  if(this->Information)
    {
    this->SetDimensions(0,0,0);
    }
}

//----------------------------------------------------------------------------
void vtkImageData::CopyInformationFromPipeline(vtkInformation* information)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationFromPipeline(information);

  this->CopyOriginAndSpacingFromPipeline(information);
}

//----------------------------------------------------------------------------
// Graphics filters reallocate every execute.  Image filters try to reuse
// the scalars.
void vtkImageData::PrepareForNewData()
{
  // free everything but the scalars
  vtkDataArray *scalars = this->GetPointData()->GetScalars();
  if (scalars)
    {
    scalars->Register(this);
    }
  this->Initialize();
  if (scalars)
    {
    this->GetPointData()->SetScalars(scalars);
    scalars->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
template <class T>
unsigned long vtkImageDataGetTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------

vtkCell *vtkImageData::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  // Use vtkIdType to avoid overflow on large images
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  vtkIdType d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return NULL;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      //cell = this->EmptyCell;
      return NULL;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell = this->Voxel;
      break;

    default:
      vtkErrorMacro("Invalid DataDescription.");
      return NULL;
    }

  // Extract point coordinates and point ids
  // Ids are relative to extent min.
  npts = 0;
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkImageData::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType npts, idx;
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  double x[3];
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    cell->SetCellTypeToEmptyCell();
    return;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      cell->SetCellTypeToEmptyCell();
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell->SetCellTypeToVertex();
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell->SetCellTypeToVoxel();
      break;
    }

  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }
}


//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkImageData::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting cell bounds from an empty image.");
    bounds[0] = bounds[1] = bounds[2] = bounds[3]
      = bounds[4] = bounds[5] = 0.0;
    return;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      break;
    }


  // carefully compute the bounds
  if (kMax >= kMin && jMax >= jMin && iMax >= iMin)
    {
    bounds[0] = bounds[2] = bounds[4] =  VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] =  VTK_DOUBLE_MIN;

    // Extract point coordinates
    for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
      {
      x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
      }
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      }
    for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
      {
      x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];
      bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
      bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
      }
    }
  else
    {
    vtkMath::UninitializeBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetPoint(vtkIdType ptId, double x[3])
{
  int i, loc[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  x[0] = x[1] = x[2] = 0.0;
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a point from an empty image.");
    return;
    }

  // "loc" holds the point x,y,z indices
  loc[0] = loc[1] = loc[2] = 0;

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return;

    case VTK_SINGLE_POINT:
      break;

    case VTK_X_LINE:
      loc[0] = ptId;
      break;

    case VTK_Y_LINE:
      loc[1] = ptId;
      break;

    case VTK_Z_LINE:
      loc[2] = ptId;
      break;

    case VTK_XY_PLANE:
      loc[0] = ptId % dims[0];
      loc[1] = ptId / dims[0];
      break;

    case VTK_YZ_PLANE:
      loc[1] = ptId % dims[1];
      loc[2] = ptId / dims[1];
      break;

    case VTK_XZ_PLANE:
      loc[0] = ptId % dims[0];
      loc[2] = ptId / dims[0];
      break;

    case VTK_XYZ_GRID:
      loc[0] = ptId % dims[0];
      loc[1] = (ptId / dims[0]) % dims[1];
      loc[2] = ptId / (dims[0]*dims[1]);
      break;
    }

  for (i=0; i<3; i++)
    {
    x[i] = origin[i] + (loc[i]+extent[i*2]) * spacing[i];
    }

  return;
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindPoint(double x[3])
{
  int i, loc[3];
  double d;
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++)
    {
    d = x[i] - origin[i];
    loc[i] = vtkMath::Floor((d / spacing[i]) + 0.5);
    if ( loc[i] < extent[i*2] || loc[i] > extent[i*2+1] )
      {
      return -1;
      }
    // since point id is relative to the first point actually stored
    loc[i] -= extent[i*2];
    }
  //
  //  From this location get the point id
  //
  return loc[2]*dims[0]*dims[1] + loc[1]*dims[0] + loc[0];

}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId),
                                 double tol2,
                                 int& subId, double pcoords[3],
                                 double *weights)
{
  return
    this->FindCell( x, NULL, 0, tol2, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                 vtkIdType vtkNotUsed(cellId),
                                 double tol2,
                                 int& subId, double pcoords[3], double *weights)
{
  int idx[3];

  // Compute the voxel index
  if ( this->ComputeStructuredCoordinates(x, idx, pcoords) == 0 )
    {
    // If voxel index is out of bounds, check point "x" against the
    // bounds to see if within tolerance of the bounds.
    const int* extent = this->Extent;
    const double* spacing = this->Spacing;
    const double* bounds = this->Bounds;

    // Compute squared distance of point x from the boundary
    double dist2 = 0.0;

    for (int i=0; i<3; i++)
      {
      int minIdx = extent[i*2];
      int maxIdx = extent[i*2+1];
      int negSpacing = (spacing[i] < 0);
      double minBound = bounds[i*2 + negSpacing];
      double maxBound = bounds[i*2 + (1-negSpacing)];

      if ( idx[i] < minIdx )
        {
        idx[i] = minIdx;
        pcoords[i] = 0.0;
        double dist = x[i] - minBound;
        dist2 += dist*dist;
        }
      else if ( idx[i] >= maxIdx )
        {
        if (maxIdx == minIdx)
          {
          idx[i] = minIdx;
          pcoords[i] = 0.0;
          }
        else
          {
          idx[i] = maxIdx-1;
          pcoords[i] = 1.0;
          }
        double dist = x[i] - maxBound;
        dist2 += dist*dist;
        }
      }

    // Check squared distance against the tolerance
    if (dist2 > tol2)
      {
      return -1;
      }
    }

  if (weights)
    {
    // Shift parametric coordinates for XZ/YZ planes
    if( this->DataDescription == VTK_XZ_PLANE )
      {
      pcoords[1] = pcoords[2];
      pcoords[2] = 0.0;
      }
    else if( this->DataDescription == VTK_YZ_PLANE )
      {
      pcoords[0] = pcoords[1];
      pcoords[1] = pcoords[2];
      pcoords[2] = 0.0;
      }
    else if( this->DataDescription == VTK_XY_PLANE )
      {
      pcoords[2] = 0.0;
      }
    vtkVoxel::InterpolationFunctions( pcoords, weights );
    }

  //
  //  From this location get the cell id
  //
  subId = 0;
  return this->ComputeCellId(idx);
}

//----------------------------------------------------------------------------
vtkCell *vtkImageData::FindAndGetCell(double x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      double tol2, int& subId,
                                      double pcoords[3], double *weights)
{
  vtkIdType cellId = this->FindCell(x, 0, 0, tol2, subId, pcoords, 0);

  if (cellId < 0)
    {
    return NULL;
    }

  vtkCell *cell = this->GetCell(cellId);
  cell->InterpolateFunctions(pcoords, weights);

  return cell;
}

//----------------------------------------------------------------------------
int vtkImageData::GetCellType(vtkIdType vtkNotUsed(cellId))
{
  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return VTK_EMPTY_CELL;

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
      return VTK_EMPTY_CELL;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::ComputeBounds()
{
  if ( this->GetMTime() <= this->ComputeTime )
    {
    return;
    }
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  if ( extent[0] > extent[1] ||
       extent[2] > extent[3] ||
       extent[4] > extent[5] )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    }
  else
    {
    int swapXBounds = (spacing[0] < 0);  // 1 if true, 0 if false
    int swapYBounds = (spacing[1] < 0);  // 1 if true, 0 if false
    int swapZBounds = (spacing[2] < 0);  // 1 if true, 0 if false

    this->Bounds[0] = origin[0] + (extent[0+swapXBounds] * spacing[0]);
    this->Bounds[2] = origin[1] + (extent[2+swapYBounds] * spacing[1]);
    this->Bounds[4] = origin[2] + (extent[4+swapZBounds] * spacing[2]);

    this->Bounds[1] = origin[0] + (extent[1-swapXBounds] * spacing[0]);
    this->Bounds[3] = origin[1] + (extent[3-swapYBounds] * spacing[1]);
    this->Bounds[5] = origin[2] + (extent[5-swapZBounds] * spacing[2]);
    }
  this->ComputeTime.Modified();
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a voxel cell, compute the eight
// gradient values for the voxel corners. The order in which the gradient
// vectors are arranged corresponds to the ordering of the voxel points.
// Gradient vector is computed by central differences (except on edges of
// volume where forward difference is used). The scalars s are the scalars
// from which the gradient is to be computed. This method will treat
// only 3D structured point datasets (i.e., volumes).
void vtkImageData::GetVoxelGradient(int i, int j, int k, vtkDataArray *s,
                                    vtkDataArray *g)
{
  double gv[3];
  int ii, jj, kk, idx=0;

  for ( kk=0; kk < 2; kk++)
    {
    for ( jj=0; jj < 2; jj++)
      {
      for ( ii=0; ii < 2; ii++)
        {
        this->GetPointGradient(i+ii, j+jj, k+kk, s, gv);
        g->SetTuple(idx++, gv);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a point in a structured point
// dataset, compute the gradient vector from the scalar data at that point.
// The scalars s are the scalars from which the gradient is to be computed.
// This method will treat structured point datasets of any dimension.
void vtkImageData::GetPointGradient(int i, int j, int k, vtkDataArray *s,
                                    double g[3])
{
  const double *ar = this->Spacing;
  double sp, sm;
  const int *extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  vtkIdType ijsize=dims[0]*dims[1];

  // Adjust i,j,k to the start of the extent
  i -= extent[0];
  j -= extent[2];
  k -= extent[4];

  // Check for out-of-bounds
  if (i < 0 || i >= dims[0] || j < 0 || j >= dims[1] || k < 0 || k >= dims[2])
    {
    g[0] = g[1] = g[2] = 0.0;
    return;
    }

  // x-direction
  if ( dims[0] == 1 )
    {
    g[0] = 0.0;
    }
  else if ( i == 0 )
    {
    sp = s->GetComponent(i+1 + j*dims[0] + k*ijsize, 0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize, 0);
    g[0] = (sm - sp) / ar[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i-1 + j*dims[0] + k*ijsize,0);
    g[0] = (sm - sp) / ar[0];
    }
  else
    {
    sp = s->GetComponent(i+1 + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i-1 + j*dims[0] + k*ijsize,0);
    g[0] = 0.5 * (sm - sp) / ar[0];
    }

  // y-direction
  if ( dims[1] == 1 )
    {
    g[1] = 0.0;
    }
  else if ( j == 0 )
    {
    sp = s->GetComponent(i + (j+1)*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    g[1] = (sm - sp) / ar[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + (j-1)*dims[0] + k*ijsize,0);
    g[1] = (sm - sp) / ar[1];
    }
  else
    {
    sp = s->GetComponent(i + (j+1)*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + (j-1)*dims[0] + k*ijsize,0);
    g[1] = 0.5 * (sm - sp) / ar[1];
    }

  // z-direction
  if ( dims[2] == 1 )
    {
    g[2] = 0.0;
    }
  else if ( k == 0 )
    {
    sp = s->GetComponent(i + j*dims[0] + (k+1)*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    g[2] = (sm - sp) / ar[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + (k-1)*ijsize,0);
    g[2] = (sm - sp) / ar[2];
    }
  else
    {
    sp = s->GetComponent(i + j*dims[0] + (k+1)*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + (k-1)*ijsize,0);
    g[2] = 0.5 * (sm - sp) / ar[2];
    }
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkImageData::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkImageData::SetDimensions(const int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}


//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkImageData::ComputeStructuredCoordinates( const double x[3], int ijk[3], double pcoords[3],
                                                const int* extent,
                                                const double* spacing,
                                                const double* origin,
                                                const double* bounds)
{
  // tolerance is needed for 2D data (this is squared tolerance)
  const double tol2 = 1e-12;
  //
  //  Compute the ijk location
  //
  int isInBounds = 1;
  for (int i = 0; i < 3; i++)
    {
    double d = x[i] - origin[i];
    double doubleLoc = d / spacing[i];
    // Floor for negative indexes.
    ijk[i] = vtkMath::Floor(doubleLoc);
    pcoords[i] = doubleLoc - static_cast<double>(ijk[i]);

    int tmpInBounds = 0;
    int minExt = extent[i*2];
    int maxExt = extent[i*2 + 1];

    // check if data is one pixel thick
    if ( minExt == maxExt )
      {
      double dist = x[i] - bounds[2*i];
      if (dist*dist <= spacing[i]*spacing[i]*tol2)
        {
        pcoords[i] = 0.0;
        ijk[i] = minExt;
        tmpInBounds = 1;
        }
      }

    // low boundary check
    else if ( ijk[i] < minExt)
      {
      if ( (spacing[i] >= 0 && x[i] >= bounds[i*2]) ||
           (spacing[i] < 0 && x[i] <= bounds[i*2 + 1]) )
        {
        pcoords[i] = 0.0;
        ijk[i] = minExt;
        tmpInBounds = 1;
        }
      }

    // high boundary check
    else if ( ijk[i] >= maxExt )
      {
      if ( (spacing[i] >= 0 && x[i] <= bounds[i*2 + 1]) ||
           (spacing[i] < 0 && x[i] >= bounds[i*2]) )
        {
        // make sure index is within the allowed cell index range
        pcoords[i] = 1.0;
        ijk[i] = maxExt - 1;
        tmpInBounds = 1;
        }
      }

    // else index is definitely within bounds
    else
      {
      tmpInBounds = 1;
      }

    // clear isInBounds if out of bounds for this dimension
    isInBounds = (isInBounds & tmpInBounds);
    }

  return isInBounds;
}

//----------------------------------------------------------------------------
int vtkImageData::ComputeStructuredCoordinates(const double x[3], int ijk[3],
                                               double pcoords[3])
{
  return ComputeStructuredCoordinates(x,ijk,pcoords,this->Extent, this->Spacing, this->Origin, this->GetBounds());
}

//----------------------------------------------------------------------------
void vtkImageData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;
  const int *dims = this->GetDimensions();
  const int* extent = this->Extent;

  os << indent << "Spacing: (" << this->Spacing[0] << ", "
                               << this->Spacing[1] << ", "
                               << this->Spacing[2] << ")\n";
  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";
  os << indent << "Dimensions: (" << dims[0] << ", "
                                  << dims[1] << ", "
                                  << dims[2] << ")\n";
  os << indent << "Increments: (" << this->Increments[0] << ", "
                                  << this->Increments[1] << ", "
                                  << this->Increments[2] << ")\n";
  os << indent << "Extent: (" << extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << extent[idx];
    }
  os << ")\n";
}


//----------------------------------------------------------------------------
void vtkImageData::SetNumberOfScalarComponents(int num,
  vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, -1, num);
}

//----------------------------------------------------------------------------
bool vtkImageData::HasNumberOfScalarComponents(vtkInformation* meta_data)
{
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data,
    FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
    {
    return false;
    }
  return scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()) != 0;
}

//----------------------------------------------------------------------------
int vtkImageData::GetNumberOfScalarComponents(vtkInformation* meta_data)
{
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data,
    FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
  if (scalarInfo && scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()))
    {
    return scalarInfo->Get( FIELD_NUMBER_OF_COMPONENTS() );
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageData::GetNumberOfScalarComponents()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (scalars)
    {
    return scalars->GetNumberOfComponents();
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkIdType *vtkImageData::GetIncrements()
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  return this->Increments;
}

//----------------------------------------------------------------------------
vtkIdType *vtkImageData::GetIncrements(vtkDataArray *scalars)
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements(scalars);

  return this->Increments;
}

//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ)
{
  vtkIdType inc[3];
  this->ComputeIncrements(inc);
  incX = inc[0];
  incY = inc[1];
  incZ = inc[2];
}

//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkDataArray *scalars,
                                 vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ)
{
  vtkIdType inc[3];
  this->ComputeIncrements(scalars, inc);
  incX = inc[0];
  incY = inc[1];
  incZ = inc[2];
}

//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkIdType inc[3])
{
  this->ComputeIncrements(inc);
}


//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkDataArray* vtkNotUsed(scalars),
                                 vtkIdType inc[3])
{
  this->ComputeIncrements(inc);
}


//----------------------------------------------------------------------------
void vtkImageData::GetContinuousIncrements(int extent[6], vtkIdType &incX,
                                           vtkIdType &incY, vtkIdType &incZ)
{
  this->GetContinuousIncrements(this->GetPointData()->GetScalars(),
                                extent, incX, incY, incZ);
}
//----------------------------------------------------------------------------
void vtkImageData::GetContinuousIncrements(vtkDataArray *scalars,
                                           int extent[6], vtkIdType &incX,
                                           vtkIdType &incY, vtkIdType &incZ)
{
  int e0, e1, e2, e3;

  incX = 0;
  const int* selfExtent = this->Extent;

  e0 = extent[0];
  if (e0 < selfExtent[0])
    {
    e0 = selfExtent[0];
    }
  e1 = extent[1];
  if (e1 > selfExtent[1])
    {
    e1 = selfExtent[1];
    }
  e2 = extent[2];
  if (e2 < selfExtent[2])
    {
    e2 = selfExtent[2];
    }
  e3 = extent[3];
  if (e3 > selfExtent[3])
    {
    e3 = selfExtent[3];
    }

  // Make sure the increments are up to date
  vtkIdType inc[3];
  this->ComputeIncrements(scalars, inc);

  incY = inc[1] - (e1 - e0 + 1)*inc[0];
  incZ = inc[2] - (e3 - e2 + 1)*inc[1];
}


//----------------------------------------------------------------------------
// This method computes the increments from the MemoryOrder and the extent.
// This version assumes we are using the Active Scalars
void vtkImageData::ComputeIncrements(vtkIdType inc[3])
{
  this->ComputeIncrements(this->GetPointData()->GetScalars(), inc);
}

//----------------------------------------------------------------------------
// This method computes the increments from the MemoryOrder and the extent.
void vtkImageData::ComputeIncrements(vtkDataArray *scalars, vtkIdType inc[3])
{
  if (!scalars)
    {
    vtkErrorMacro("No Scalar Field has been specified - assuming 1 component!");
    this->ComputeIncrements(1, inc);
    }
  else
    {
    this->ComputeIncrements(scalars->GetNumberOfComponents(), inc);
    }
}
//----------------------------------------------------------------------------
// This method computes the increments from the MemoryOrder and the extent.
void vtkImageData::ComputeIncrements(int numberOfComponents, vtkIdType inc[3])
{
  int idx;
  vtkIdType incr = numberOfComponents;
  const int* extent = this->Extent;

  for (idx = 0; idx < 3; ++idx)
    {
    inc[idx] = incr;
    incr *= (extent[idx*2+1] - extent[idx*2] + 1);
    }
}

//----------------------------------------------------------------------------
void vtkImageData::CopyOriginAndSpacingFromPipeline(vtkInformation* info)
{
  // Copy origin and spacing from pipeline information to the internal
  // copies.
  if(info->Has(SPACING()))
    {
    this->SetSpacing(info->Get(SPACING()));
    }
  if(info->Has(ORIGIN()))
    {
    this->SetOrigin(info->Get(ORIGIN()));
    }
}

//----------------------------------------------------------------------------
template <class TIn, class TOut>
void vtkImageDataConvertScalar(TIn* in, TOut* out)
{
  *out = static_cast<TOut>(*in);
}

//----------------------------------------------------------------------------
double vtkImageData::GetScalarComponentAsDouble(int x, int y, int z, int comp)
{
  // Check the component index.
  if(comp < 0 || comp >= this->GetNumberOfScalarComponents())
    {
    vtkErrorMacro("Bad component index " << comp);
    return 0.0;
    }

  // Get a pointer to the scalar tuple.
  void* ptr = this->GetScalarPointer(x, y, z);
  if(!ptr)
    {
    // An error message was already generated by GetScalarPointer.
    return 0.0;
    }
  double result = 0.0;

  // Convert the scalar type.
  int scalarType = this->GetPointData()->GetScalars()->GetDataType();
  switch (scalarType)
    {
    vtkTemplateMacro(vtkImageDataConvertScalar(static_cast<VTK_TT*>(ptr)+comp,
                                               &result));
    default:
      {
      vtkErrorMacro("Unknown Scalar type " << scalarType);
      }
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarComponentFromDouble(int x, int y, int z, int comp,
                                                double value)
{
  // Check the component index.
  if(comp < 0 || comp >= this->GetNumberOfScalarComponents())
    {
    vtkErrorMacro("Bad component index " << comp);
    return;
    }

  // Get a pointer to the scalar tuple.
  void* ptr = this->GetScalarPointer(x, y, z);
  if(!ptr)
    {
    // An error message was already generated by GetScalarPointer.
    return;
    }

  // Convert the scalar type.
  int scalarType = this->GetPointData()->GetScalars()->GetDataType();
  switch (scalarType)
    {
    vtkTemplateMacro(vtkImageDataConvertScalar(
                       &value, static_cast<VTK_TT*>(ptr)+comp));
    default:
      {
      vtkErrorMacro("Unknown Scalar type " << scalarType);
      }
    }
}

//----------------------------------------------------------------------------
float vtkImageData::GetScalarComponentAsFloat(int x, int y, int z, int comp)
{
  return this->GetScalarComponentAsDouble(x, y, z, comp);
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarComponentFromFloat(int x, int y, int z, int comp,
                                               float value)
{
  this->SetScalarComponentFromDouble(x, y, z, comp, value);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int x, int y, int z)
{
  int tmp[3];
  tmp[0] = x;
  tmp[1] = y;
  tmp[2] = z;
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointerForExtent(int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
void *vtkImageData::GetScalarPointer(int coordinate[3])
{
  vtkDataArray *scalars = this->GetPointData()->GetScalars();

  // Make sure the array has been allocated.
  if (scalars == NULL)
    {
    //vtkDebugMacro("Allocating scalars in ImageData");
    //abort();
    //this->AllocateScalars();
    //scalars = this->PointData->GetScalars();
    return NULL;
    }

  const int* extent = this->Extent;
  // error checking: since most access will be from pointer arithmetic.
  // this should not waste much time.
  for (int idx = 0; idx < 3; ++idx)
    {
    if (coordinate[idx] < extent[idx*2] ||
        coordinate[idx] > extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinate[0] << ", "
        << coordinate[1] << ", "
        << coordinate[2] << ") not in memory.\n Current extent= ("
        << extent[0] << ", " << extent[1] << ", "
        << extent[2] << ", " << extent[3] << ", "
        << extent[4] << ", " << extent[5] << ")");
      return NULL;
      }
    }

  return this->GetArrayPointer(scalars, coordinate);
}

//----------------------------------------------------------------------------
// This method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetScalarPointer()
{
  if (this->PointData->GetScalars() == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    abort();
    //this->AllocateScalars();
    }
  return this->PointData->GetScalars()->GetVoidPointer(0);
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarType(int type, vtkInformation* meta_data)
{
  vtkDataObject::SetPointDataActiveScalarInfo(meta_data, type, -1);
}

//----------------------------------------------------------------------------
int vtkImageData::GetScalarType()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (!scalars)
    {
    return VTK_DOUBLE;
    }
  return scalars->GetDataType();
}

//----------------------------------------------------------------------------
bool vtkImageData::HasScalarType(vtkInformation* meta_data)
{
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data,
    FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
  if (!scalarInfo)
    {
    return false;
    }

  return scalarInfo->Has( FIELD_ARRAY_TYPE() ) != 0;
}

//----------------------------------------------------------------------------
int vtkImageData::GetScalarType(vtkInformation* meta_data)
{
  vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
    meta_data,
    FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);
  if (scalarInfo)
    {
    return scalarInfo->Get( FIELD_ARRAY_TYPE() );
    }
  return VTK_DOUBLE;
}

//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars(vtkInformation* pipeline_info)
{
  int newType = VTK_DOUBLE;
  int newNumComp = 1;

  if(pipeline_info)
    {
    vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(
      pipeline_info,
      FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (scalarInfo)
      {
      newType = scalarInfo->Get( FIELD_ARRAY_TYPE() );
      if ( scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()) )
        {
        newNumComp = scalarInfo->Get( FIELD_NUMBER_OF_COMPONENTS() );
        }
      }
    }

  this->AllocateScalars(newType, newNumComp);
}

//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars(int dataType, int numComponents)
{
  vtkDataArray *scalars;

  // if the scalar type has not been set then we have a problem
  if (dataType == VTK_VOID)
    {
    vtkErrorMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
    }

  const int* extent = this->Extent;
  // Use vtkIdType to avoid overflow on large images
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType imageSize = dims[0]*dims[1]*dims[2];

  // if we currently have scalars then just adjust the size
  scalars = this->PointData->GetScalars();
  if (scalars && scalars->GetDataType() == dataType
      && scalars->GetReferenceCount() == 1)
    {
    scalars->SetNumberOfComponents(numComponents);
    scalars->SetNumberOfTuples(imageSize);
    // Since the execute method will be modifying the scalars
    // directly.
    scalars->Modified();
    return;
    }

  // allocate the new scalars
  scalars = vtkDataArray::CreateDataArray(dataType);
  scalars->SetNumberOfComponents(numComponents);
  scalars->SetName("ImageScalars");

  // allocate enough memory
  scalars->SetNumberOfTuples(imageSize);

  this->PointData->SetScalars(scalars);
  scalars->Delete();
}


//----------------------------------------------------------------------------
int vtkImageData::GetScalarSize(vtkInformation* meta_data)
{
  return vtkDataArray::GetDataTypeSize(this->GetScalarType(meta_data));
}

int vtkImageData::GetScalarSize()
{
  vtkDataArray* scalars = this->GetPointData()->GetScalars();
  if (!scalars)
    {
    return vtkDataArray::GetDataTypeSize(VTK_DOUBLE);
    }
  return vtkDataArray::GetDataTypeSize(scalars->GetDataType());
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkImageDataCastExecute(vtkImageData *inData, IT *inPtr,
                             vtkImageData *outData, OT *outPtr,
                             int outExt[6])
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int rowLength;

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
        *outPtr = static_cast<OT>(*inPtr);
        outPtr++;
        inPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}



//----------------------------------------------------------------------------
template <class T>
void vtkImageDataCastExecute(vtkImageData *inData, T *inPtr,
                             vtkImageData *outData, int outExt[6])
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  if (outPtr == NULL)
    {
    vtkGenericWarningMacro("Scalars not allocated.");
    return;
    }

  int scalarType = outData->GetPointData()->GetScalars()->GetDataType();
  switch (scalarType)
    {
    vtkTemplateMacro(
      vtkImageDataCastExecute(inData,
                              static_cast<T *>(inPtr),
                              outData,
                              static_cast<VTK_TT *>(outPtr),
                              outExt) );
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageData::CopyAndCastFrom(vtkImageData *inData, int extent[6])
{
  void *inPtr = inData->GetScalarPointerForExtent(extent);

  if (inPtr == NULL)
    {
    vtkErrorMacro("Scalars not allocated.");
    return;
    }

  int scalarType = inData->GetPointData()->GetScalars()->GetDataType();
  switch (scalarType)
    {
    vtkTemplateMacro(vtkImageDataCastExecute(inData,
                                             static_cast<VTK_TT *>(inPtr),
                                             this, extent) );
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::Crop(const int* updateExtent)
{
  int           nExt[6];
  int           idxX, idxY, idxZ;
  int           maxX, maxY, maxZ;
  vtkIdType     outId, inId, inIdY, inIdZ, incZ, incY;
  vtkImageData  *newImage;
  vtkIdType numPts, numCells, tmp;
  const int* extent = this->Extent;

  // If extents already match, then we need to do nothing.
  if (extent[0] == updateExtent[0]
      && extent[1] == updateExtent[1]
      && extent[2] == updateExtent[2]
      && extent[3] == updateExtent[3]
      && extent[4] == updateExtent[4]
      && extent[5] == updateExtent[5])
    {
    return;
    }

  // Take the intersection of the two extent so that
  // we are not asking for more than the extent.
  memcpy(nExt, updateExtent, 6*sizeof(int));
  if (nExt[0] < extent[0]) { nExt[0] = extent[0];}
  if (nExt[1] > extent[1]) { nExt[1] = extent[1];}
  if (nExt[2] < extent[2]) { nExt[2] = extent[2];}
  if (nExt[3] > extent[3]) { nExt[3] = extent[3];}
  if (nExt[4] < extent[4]) { nExt[4] = extent[4];}
  if (nExt[5] > extent[5]) { nExt[5] = extent[5];}

  // If the extents are the same just return.
  if (extent[0] == nExt[0] && extent[1] == nExt[1]
      && extent[2] == nExt[2] && extent[3] == nExt[3]
      && extent[4] == nExt[4] && extent[5] == nExt[5])
    {
    vtkDebugMacro("Extents already match.");
    return;
    }

  // How many point/cells.
  numPts = (nExt[1]-nExt[0]+1)*(nExt[3]-nExt[2]+1)*(nExt[5]-nExt[4]+1);
  // Conditional are to handle 3d, 2d, and even 1d images.
  tmp = nExt[1] - nExt[0];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells = tmp;
  tmp = nExt[3] - nExt[2];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells *= tmp;
  tmp = nExt[5] - nExt[4];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells *= tmp;

  // Create a new temporary image.
  newImage = vtkImageData::New();
  newImage->SetExtent(nExt);
  vtkPointData *npd = newImage->GetPointData();
  vtkCellData *ncd = newImage->GetCellData();
  npd->CopyAllocate(this->PointData, numPts);
  ncd->CopyAllocate(this->CellData, numCells);

  // Loop through outData points
  incY = extent[1]-extent[0]+1;
  incZ = (extent[3]-extent[2]+1)*incY;
  outId = 0;
  inIdZ = incZ * (nExt[4]-extent[4])
          + incY * (nExt[2]-extent[2])
          + (nExt[0]-extent[0]);

  for (idxZ = nExt[4]; idxZ <= nExt[5]; idxZ++)
    {
    inIdY = inIdZ;
    for (idxY = nExt[2]; idxY <= nExt[3]; idxY++)
      {
      inId = inIdY;
      for (idxX = nExt[0]; idxX <= nExt[1]; idxX++)
        {
        npd->CopyData( this->PointData, inId, outId);
        ++inId;
        ++outId;
        }
      inIdY += incY;
      }
    inIdZ += incZ;
    }

  // Loop through outData cells
  // Have to handle the 2d and 1d cases.
  maxX = nExt[1];
  maxY = nExt[3];
  maxZ = nExt[5];
  if (maxX == nExt[0])
    {
    ++maxX;
    }
  if (maxY == nExt[2])
    {
    ++maxY;
    }
  if (maxZ == nExt[4])
    {
    ++maxZ;
    }
  incY = extent[1]-extent[0];
  incZ = (extent[3]-extent[2])*incY;
  outId = 0;
  inIdZ = incZ * (nExt[4]-extent[4])
          + incY * (nExt[2]-extent[2])
          + (nExt[0]-extent[0]);
  for (idxZ = nExt[4]; idxZ < maxZ; idxZ++)
    {
    inIdY = inIdZ;
    for (idxY = nExt[2]; idxY < maxY; idxY++)
      {
      inId = inIdY;
      for (idxX = nExt[0]; idxX < maxX; idxX++)
        {
        ncd->CopyData(this->CellData, inId, outId);
        ++inId;
        ++outId;
        }
      inIdY += incY;
      }
    inIdZ += incZ;
    }

  this->PointData->ShallowCopy(npd);
  this->CellData->ShallowCopy(ncd);
  this->SetExtent(nExt);
  newImage->Delete();
}



//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMin(vtkInformation* meta_data)
{
  return vtkDataArray::GetDataTypeMin(this->GetScalarType(meta_data));
}

//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetScalarType());
}


//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMax(vtkInformation* meta_data)
{
  return vtkDataArray::GetDataTypeMax(this->GetScalarType(meta_data));
}

//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetScalarType());
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
  this->SetExtent(ext);
}

//----------------------------------------------------------------------------
void vtkImageData::SetDataDescription(int desc)
{
  if (desc == this->DataDescription)
    {
    return;
    }

  this->DataDescription = desc;

  if (this->Vertex)
    {
    this->Vertex->Delete();
    this->Vertex = 0;
    }
  if (this->Line)
    {
    this->Line->Delete();
    this->Line = 0;
    }
  if (this->Pixel)
    {
    this->Pixel->Delete();
    this->Pixel = 0;
    }
  if (this->Voxel)
    {
    this->Voxel->Delete();
    this->Voxel = 0;
    }
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT:
      this->Vertex = vtkVertex::New();
      break;

    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      this->Line = vtkLine::New();
      break;

    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      this->Pixel = vtkPixel::New();
      break;

    case VTK_XYZ_GRID:
      this->Voxel = vtkVoxel::New();;
      break;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int *extent)
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if ( description < 0 ) //improperly specified
    {
    vtkErrorMacro (<< "Bad Extent, retaining previous values");
    }

  if (description == VTK_UNCHANGED)
    {
    return;
    }

  this->SetDataDescription(description);

  this->Modified();
}



//----------------------------------------------------------------------------
int *vtkImageData::GetDimensions()
{
  this->GetDimensions(this->Dimensions);
  return this->Dimensions;
}

//----------------------------------------------------------------------------
void vtkImageData::GetDimensions(int *dOut)
{
  const int* extent = this->Extent;
  dOut[0] = extent[1] - extent[0] + 1;
  dOut[1] = extent[3] - extent[2] + 1;
  dOut[2] = extent[5] - extent[4] + 1;
}

//----------------------------------------------------------------------------
void vtkImageData::SetAxisUpdateExtent(int idx, int min, int max,
                                       const int* updateExtent,
                                       int* axisUpdateExtent)
{
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }

  memcpy(axisUpdateExtent, updateExtent, 6*sizeof(int));
  if (axisUpdateExtent[idx*2] != min)
    {
    axisUpdateExtent[idx*2] = min;
    }
  if (axisUpdateExtent[idx*2+1] != max)
    {
    axisUpdateExtent[idx*2+1] = max;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxisUpdateExtent(int idx, int &min, int &max,
                                       const int* updateExtent)
{
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }

  min = updateExtent[idx*2];
  max = updateExtent[idx*2+1];
}


//----------------------------------------------------------------------------
unsigned long vtkImageData::GetActualMemorySize()
{
  return this->vtkDataSet::GetActualMemorySize();
}


//----------------------------------------------------------------------------
void vtkImageData::ShallowCopy(vtkDataObject *dataObject)
{
  vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);

  if ( imageData != NULL )
    {
    this->InternalImageDataCopy(imageData);
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkImageData::DeepCopy(vtkDataObject *dataObject)
{
  vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);

  if ( imageData != NULL )
    {
    this->InternalImageDataCopy(imageData);
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkImageData::InternalImageDataCopy(vtkImageData *src)
{
  int idx;

  //this->SetScalarType(src->GetScalarType());
  //this->SetNumberOfScalarComponents(src->GetNumberOfScalarComponents());
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    this->Increments[idx] = src->Increments[idx];
    this->Origin[idx] = src->Origin[idx];
    this->Spacing[idx] = src->Spacing[idx];
    }
  this->SetExtent(src->GetExtent());
}



//----------------------------------------------------------------------------
vtkIdType vtkImageData::GetNumberOfCells()
{
  vtkIdType nCells=1;
  int i;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  for (i=0; i<3; i++)
    {
    if (dims[i] == 0)
      {
      return 0;
      }
    if (dims[i] > 1)
      {
      nCells *= (dims[i]-1);
      }
    }

  return nCells;
}

//============================================================================
// Starting to make some more general methods that deal with any array
// (not just scalars).
//============================================================================


//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void vtkImageData::GetArrayIncrements(vtkDataArray* array, vtkIdType increments[3])
{
  const int* extent = this->Extent;
  // We could store tuple increments and just
  // multiply by the number of components...
  increments[0] = array->GetNumberOfComponents();
  increments[1] = increments[0] * (extent[1]-extent[0]+1);
  increments[2] = increments[1] * (extent[3]-extent[2]+1);
}

//----------------------------------------------------------------------------
void *vtkImageData::GetArrayPointerForExtent(vtkDataArray* array,
                                             int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetArrayPointer(array, tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetArrayPointer(vtkDataArray* array, int coordinate[3])
{
  vtkIdType incs[3];
  vtkIdType idx;

  if (array == NULL)
    {
    return NULL;
    }

  const int* extent = this->Extent;
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < 3; ++idx)
    {
    if (coordinate[idx] < extent[idx*2] ||
        coordinate[idx] > extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetPointer: Pixel (" << coordinate[0] << ", "
        << coordinate[1] << ", "
        << coordinate[2] << ") not in current extent: ("
        << extent[0] << ", " << extent[1] << ", "
        << extent[2] << ", " << extent[3] << ", "
        << extent[4] << ", " << extent[5] << ")");
      return NULL;
      }
    }

  // compute the index of the vector.
  this->GetArrayIncrements(array, incs);
  idx = ((coordinate[0] - extent[0]) * incs[0]
         + (coordinate[1] - extent[2]) * incs[1]
         + (coordinate[2] - extent[4]) * incs[2]);
  // I could check to see if the array has the correct number
  // of tuples for the extent, but that would be an extra multiply.
  if (idx < 0 || idx > array->GetMaxId())
    {
    vtkErrorMacro("Coordinate (" << coordinate[0] << ", " << coordinate[1]
                  << ", " << coordinate[2] << ") out side of array (max = "
                  << array->GetMaxId());
    return NULL;
    }

  return array->GetVoidPointer(idx);
}


//----------------------------------------------------------------------------
void vtkImageData::ComputeInternalExtent(int *intExt, int *tgtExt, int *bnds)
{
  int i;
  const int* extent = this->Extent;
  for (i = 0; i < 3; ++i)
    {
    intExt[i*2] = tgtExt[i*2];
    if (intExt[i*2] - bnds[i*2] < extent[i*2])
      {
      intExt[i*2] = extent[i*2] + bnds[i*2];
      }
    intExt[i*2+1] = tgtExt[i*2+1];
    if (intExt[i*2+1] + bnds[i*2+1] > extent[i*2+1])
      {
      intExt[i*2+1] = extent[i*2+1] - bnds[i*2+1];
      }
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageData::GetData(vtkInformation* info)
{
  return info? vtkImageData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageData::GetData(vtkInformationVector* v, int i)
{
  return vtkImageData::GetData(v->GetInformationObject(i));
}
