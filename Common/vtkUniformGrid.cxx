/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformGrid.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkLargeInteger.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredVisibilityConstraint.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"

vtkCxxRevisionMacro(vtkUniformGrid, "1.3");
vtkStandardNewMacro(vtkUniformGrid);

vtkCxxSetObjectMacro(vtkUniformGrid,
                     PointVisibility,
                     vtkStructuredVisibilityConstraint);
vtkCxxSetObjectMacro(vtkUniformGrid,
                     CellVisibility,
                     vtkStructuredVisibilityConstraint);

//----------------------------------------------------------------------------
vtkUniformGrid::vtkUniformGrid()
{
  int idx;
  
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();
  this->EmptyCell = vtkEmptyCell::New();
  
  this->DataDescription = VTK_EMPTY;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = 0;
    this->Extent[idx*2] = 0;
    this->Extent[idx*2+1] = -1;    
    this->Origin[idx] = 0.0;
    this->Spacing[idx] = 1.0;
    }

  this->PointVisibility = vtkStructuredVisibilityConstraint::New();
  this->CellVisibility = vtkStructuredVisibilityConstraint::New();
}

//----------------------------------------------------------------------------
vtkUniformGrid::~vtkUniformGrid()
{
  this->Vertex->Delete();
  this->Line->Delete();
  this->Pixel->Delete();
  this->Voxel->Delete();
  this->EmptyCell->Delete();

  this->PointVisibility->Delete();
  this->CellVisibility->Delete();
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points 
// object.
void vtkUniformGrid::CopyStructure(vtkDataSet *ds)
{
  this->Initialize();

  vtkUniformGrid *sPts=vtkUniformGrid::SafeDownCast(ds);
  if (!sPts)
    {
    return;
    }

  for (int i=0; i<3; i++)
    {
    this->Extent[i] = sPts->Extent[i];
    this->Extent[i+3] = sPts->Extent[i+3];
    this->Dimensions[i] = sPts->Dimensions[i];
    this->Spacing[i] = sPts->Spacing[i];
    this->Origin[i] = sPts->Origin[i];
    }
  this->DataDescription = sPts->DataDescription;
  this->CopyInformation(sPts);

  this->PointVisibility->ShallowCopy(sPts->PointVisibility);
  this->CellVisibility->ShallowCopy(sPts->CellVisibility);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::Initialize()
{
  this->Superclass::Initialize();

  this->PointVisibility->Delete();
  this->PointVisibility = vtkStructuredVisibilityConstraint::New();

  this->CellVisibility->Delete();
  this->CellVisibility = vtkStructuredVisibilityConstraint::New();
}

//----------------------------------------------------------------------------
vtkCell *vtkUniformGrid::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  double x[3];
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return this->EmptyCell;
    }
  
  // see whether the cell is blanked
  if ( (this->PointVisibility->IsConstrained() || 
        this->CellVisibility->IsConstrained())
       && !this->IsCellVisible(cellId) )
    {
    return this->EmptyCell;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      return this->EmptyCell;

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
    }

  // Extract point coordinates and point ids
  // Ids are relative to extent min.
  npts = 0;
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkUniformGrid::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType npts, idx;
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  double x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    cell->SetCellTypeToEmptyCell();
    return;
    }
  
  // see whether the cell is blanked
  if ( (this->PointVisibility->IsConstrained() || 
        this->CellVisibility->IsConstrained())
       && !this->IsCellVisible(cellId) )
    {
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
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

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
void vtkUniformGrid::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

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
    bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;
  
    // Extract point coordinates
    for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
      {
      x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
      }
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      }
    for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
      {
      x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 
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
double *vtkUniformGrid::GetPoint(vtkIdType ptId)
{
  static double x[3];
  int i, loc[3];
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

  x[0] = x[1] = x[2] = 0.0;
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a point from an empty image.");
    return x;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      return x;

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
      loc[0] = ptId % dims[0];
      loc[1] = ptId / dims[0];
      break;

    case VTK_YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % dims[1];
      loc[2] = ptId / dims[1];
      break;

    case VTK_XZ_PLANE:
      loc[1] = 0;
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
    x[i] = origin[i] + (loc[i]+this->Extent[i*2]) * spacing[i];
    }

  return x;
}

//----------------------------------------------------------------------------
vtkIdType vtkUniformGrid::FindPoint(double x[3])
{
  int i, loc[3];
  double d;
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    loc[i] = (int) ((d / spacing[i]) + 0.5);
    if ( loc[i] < this->Extent[i*2] || loc[i] > this->Extent[i*2+1] )
      {
      return -1;
      } 
    // since point id is relative to the first point actually stored
    loc[i] -= this->Extent[i*2];
    }
  //
  //  From this location get the point id
  //
  return loc[2]*dims[0]*dims[1] + loc[1]*dims[0] + loc[0];
  
}

//----------------------------------------------------------------------------
vtkIdType vtkUniformGrid::FindCell(double x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId), 
                                  double vtkNotUsed(tol2), 
                                  int& subId, double pcoords[3], 
                                  double *weights)
{
  return
    this->FindCell( x, (vtkCell *)NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkUniformGrid::FindCell(double x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkIdType vtkNotUsed(cellId),
                                 double vtkNotUsed(tol2), 
                                 int& subId, double pcoords[3], double *weights)
{
  int loc[3];
  int *dims = this->GetDimensions();

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return -1;
    }

  vtkVoxel::InterpolationFunctions(pcoords,weights);

  //
  //  From this location get the cell id
  //
  subId = 0;
  vtkIdType cellId = loc[2] * (dims[0]-1)*(dims[1]-1) +
    loc[1] * (dims[0]-1) + loc[0];

  if ( (this->PointVisibility->IsConstrained() || 
        this->CellVisibility->IsConstrained())
       && !this->IsCellVisible(cellId) )
    {
    return -1;
    }
  return cellId;
  
}

//----------------------------------------------------------------------------
vtkCell *vtkUniformGrid::FindAndGetCell(double x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      double vtkNotUsed(tol2), int& subId, 
                                      double pcoords[3], double *weights)
{
  int i, j, k, loc[3];
  vtkIdType npts, idx;
  int *dims = this->GetDimensions();
  vtkIdType d01 = dims[0]*dims[1];
  double xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return NULL;
    }

  vtkIdType cellId = loc[2] * (dims[0]-1)*(dims[1]-1) +
    loc[1] * (dims[0]-1) + loc[0];

  if ( (this->PointVisibility->IsConstrained() || 
        this->CellVisibility->IsConstrained())
       && !this->IsCellVisible(cellId) )
    {
    return NULL;
    }

  //
  // Get the parametric coordinates and weights for interpolation
  //
  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return NULL;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      vtkVertex::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      vtkVoxel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Voxel;
      break;
    }

  npts = 0;
  for (k = loc[2]; k <= kMax; k++)
    {
    xOut[2] = origin[2] + k * spacing[2]; 
    for (j = loc[1]; j <= jMax; j++)
      {
      xOut[1] = origin[1] + j * spacing[1]; 
      // make idx relative to the extent not the whole extent
      idx = loc[0]-this->Extent[0] + (j-this->Extent[2])*dims[0]
        + (k-this->Extent[4])*d01;
      for (i = loc[0]; i <= iMax; i++, idx++)
        {
        xOut[0] = origin[0] + i * spacing[0]; 

        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,xOut);
        }
      }
    }
  subId = 0;

  return cell;
}

//----------------------------------------------------------------------------
int vtkUniformGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  if ( (this->PointVisibility->IsConstrained() || 
        this->CellVisibility->IsConstrained())
       && !this->IsCellVisible(cellId) )
    {
    return VTK_EMPTY_CELL;
    }

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
void vtkUniformGrid::ComputeBounds()
{
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  
  if ( this->Extent[0] > this->Extent[1] || 
       this->Extent[2] > this->Extent[3] ||
       this->Extent[4] > this->Extent[5] )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }
  this->Bounds[0] = origin[0] + (this->Extent[0] * spacing[0]);
  this->Bounds[2] = origin[1] + (this->Extent[2] * spacing[1]);
  this->Bounds[4] = origin[2] + (this->Extent[4] * spacing[2]);

  this->Bounds[1] = origin[0] + (this->Extent[1] * spacing[0]);
  this->Bounds[3] = origin[1] + (this->Extent[3] * spacing[1]);
  this->Bounds[5] = origin[2] + (this->Extent[5] * spacing[2]);
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkUniformGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkUniformGrid::SetDimensions(int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}


// streaming change: ijk is in extent coordinate system.
//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkUniformGrid::ComputeStructuredCoordinates(double x[3], int ijk[3], 
                                               double pcoords[3])
{
  int i;
  double d, doubleLoc;
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();
  
  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    doubleLoc = d / spacing[i];
    // Floor for negtive indexes.
    ijk[i] = (int) (floor(doubleLoc));
    if ( ijk[i] >= this->Extent[i*2] && ijk[i] < this->Extent[i*2 + 1] )
      {
      pcoords[i] = doubleLoc - (double)ijk[i];
      }

    else if ( ijk[i] < this->Extent[i*2] || ijk[i] > this->Extent[i*2+1] ) 
      {
      return 0;
      } 

    else //if ( ijk[i] == this->Extent[i*2+1] )
      {
      if (dims[i] == 1)
        {
        pcoords[i] = 0.0;
        }
      else
        {
        ijk[i] -= 1;
        pcoords[i] = 1.0;
        }
      }

    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkUniformGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;
  int *dims = this->GetDimensions();
  
  os << indent << "Spacing: (" << this->Spacing[0] << ", "
                               << this->Spacing[1] << ", "
                               << this->Spacing[2] << ")\n";
  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";
  os << indent << "Dimensions: (" << dims[0] << ", "
                                  << dims[1] << ", "
                                  << dims[2] << ")\n";
  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
void vtkUniformGrid::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
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
int *vtkUniformGrid::GetDimensions()
{
  this->Dimensions[0] = this->Extent[1] - this->Extent[0] + 1;
  this->Dimensions[1] = this->Extent[3] - this->Extent[2] + 1;
  this->Dimensions[2] = this->Extent[5] - this->Extent[4] + 1;

  return this->Dimensions;
}

//----------------------------------------------------------------------------
void vtkUniformGrid::GetDimensions(int *dOut)
{
  int *dims = this->GetDimensions();
  dOut[0] = dims[0];
  dOut[1] = dims[1];
  dOut[2] = dims[2];  
}

//----------------------------------------------------------------------------
void vtkUniformGrid::SetExtent(int *extent)
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

  this->DataDescription = description;
  
  this->Modified();
}


//----------------------------------------------------------------------------
unsigned long vtkUniformGrid::GetActualMemorySize()
{
  return this->vtkDataSet::GetActualMemorySize();
}


//----------------------------------------------------------------------------
void vtkUniformGrid::ShallowCopy(vtkDataObject *dataObject)
{
  vtkUniformGrid *ugData = vtkUniformGrid::SafeDownCast(dataObject);

  if ( ugData )
    {
    this->InternalUniformGridCopy(ugData);
    this->PointVisibility->ShallowCopy(ugData->PointVisibility);
    this->CellVisibility->ShallowCopy(ugData->CellVisibility);
    }
  else
    {
    vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);
    if (imageData)
      {
      this->InternalUniformGridCopy(imageData);
      }
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::DeepCopy(vtkDataObject *dataObject)
{
  vtkUniformGrid *ugData = vtkUniformGrid::SafeDownCast(dataObject);

  if ( ugData != NULL )
    {
    this->InternalUniformGridCopy(ugData);
    this->PointVisibility->DeepCopy(ugData->PointVisibility);
    this->CellVisibility->DeepCopy(ugData->CellVisibility);
    }
  else
    {
    vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);
    if (imageData)
      {
      this->InternalUniformGridCopy(imageData);
      }
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkUniformGrid::InternalUniformGridCopy(vtkUniformGrid *src)
{
  int idx;

  this->DataDescription = src->DataDescription;
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    this->Origin[idx] = src->Origin[idx];
    this->Spacing[idx] = src->Spacing[idx];
    }
}

void vtkUniformGrid::InternalUniformGridCopy(vtkImageData *src)
{
  int idx;

  double origin[3];
  double spacing[3];
  src->GetOrigin(origin);
  src->GetSpacing(spacing);
  this->SetExtent(src->GetExtent());
  for (idx = 0; idx < 3; ++idx)
    {
    this->Origin[idx] = origin[idx];
    this->Spacing[idx] = spacing[idx];
    }
}


//----------------------------------------------------------------------------
vtkIdType vtkUniformGrid::GetNumberOfCells() 
{
  vtkIdType nCells=1;
  int i;
  int *dims = this->GetDimensions();

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

//----------------------------------------------------------------------------
// Override this method because of blanking
void vtkUniformGrid::GetScalarRange(double range[2])
{
  vtkDataArray *ptScalars = this->PointData->GetScalars();
  vtkDataArray *cellScalars = this->CellData->GetScalars();
  double ptRange[2];
  double cellRange[2];
  double s;
  int id, num;
  
  ptRange[0] =  VTK_DOUBLE_MAX;
  ptRange[1] =  -VTK_DOUBLE_MAX;
  if ( ptScalars )
    {
    num = this->GetNumberOfPoints();
    for (id=0; id < num; id++)
      {
      if ( this->IsPointVisible(id) )
        {
        s = ptScalars->GetComponent(id,0);
        if ( s < ptRange[0] )
          {
          ptRange[0] = s;
          }
        if ( s > ptRange[1] )
          {
          ptRange[1] = s;
          }
        }
      }
    }

  cellRange[0] =  ptRange[0];
  cellRange[1] =  ptRange[1];
  if ( cellScalars )
    {
    num = this->GetNumberOfCells();
    for (id=0; id < num; id++)
      {
      if ( this->IsCellVisible(id) )
        {
        s = cellScalars->GetComponent(id,0);
        if ( s < cellRange[0] )
          {
          cellRange[0] = s;
          }
        if ( s > cellRange[1] )
          {
          cellRange[1] = s;
          }
        }
      }
    }

  range[0] = (cellRange[0] >= VTK_DOUBLE_MAX ? 0.0 : cellRange[0]);
  range[1] = (cellRange[1] <= -VTK_DOUBLE_MAX ? 1.0 : cellRange[1]);

  this->ComputeTime.Modified();
}

//----------------------------------------------------------------------------
// Turn off a particular data point.
void vtkUniformGrid::BlankPoint(vtkIdType ptId)
{
  this->PointVisibility->Initialize(this->Dimensions);
  this->PointVisibility->Blank(ptId);
}

//----------------------------------------------------------------------------
// Turn on a particular data point.
void vtkUniformGrid::UnBlankPoint(vtkIdType ptId)
{
  this->PointVisibility->Initialize(this->Dimensions);
  this->PointVisibility->UnBlank(ptId);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::SetPointVisibilityArray(vtkUnsignedCharArray *ptVis)
{
  this->PointVisibility->SetVisibilityById(ptVis);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUniformGrid::GetPointVisibilityArray()
{
  return this->PointVisibility->GetVisibilityById();
}

//----------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkUniformGrid::BlankCell(vtkIdType cellId)
{
  this->CellVisibility->Initialize(this->Dimensions);
  this->CellVisibility->Blank(cellId);
}

//----------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkUniformGrid::UnBlankCell(vtkIdType cellId)
{
  this->CellVisibility->Initialize(this->Dimensions);
  this->CellVisibility->UnBlank(cellId);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::SetCellVisibilityArray(vtkUnsignedCharArray *cellVis)
{
  this->CellVisibility->SetVisibilityById(cellVis);
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray* vtkUniformGrid::GetCellVisibilityArray()
{
  return this->CellVisibility->GetVisibilityById();
}

//----------------------------------------------------------------------------
unsigned char vtkUniformGrid::IsPointVisible(vtkIdType pointId)
{
  return this->PointVisibility->IsVisible(pointId);
}

//----------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkUniformGrid::IsCellVisible(vtkIdType cellId)
{

  if ( !this->CellVisibility->IsVisible(cellId) )
    {
    return 0;
    }

  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  switch (this->DataDescription)
    {
    case VTK_EMPTY: 
      return 0;

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

  // Extract point ids
  // Ids are relative to extent min.
  vtkIdType idx[8];
  vtkIdType npts = 0;
  int loc[3];
  int d01 = dims[0]*dims[1];
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        idx[npts] = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        npts++;
        }
      }
    }

  for (int i=0; i<npts; i++)
    {
    if ( !this->IsPointVisible(idx[i]) )
      {
      return 0;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
unsigned char vtkUniformGrid::GetPointBlanking()
{
  return this->PointVisibility->IsConstrained();
}

//----------------------------------------------------------------------------
unsigned char vtkUniformGrid::GetCellBlanking()
{
  return this->PointVisibility->IsConstrained() || 
    this->CellVisibility->IsConstrained();
}

//----------------------------------------------------------------------------
void vtkUniformGrid::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
  this->UpdateExtentInitialized = 1;
}
