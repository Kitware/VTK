/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkRectilinearGrid.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkVoxel.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkRectilinearGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRectilinearGrid");
  if(ret)
    {
    return (vtkRectilinearGrid*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRectilinearGrid;
}




//----------------------------------------------------------------------------
vtkRectilinearGrid::vtkRectilinearGrid()
{
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();
  
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;

  vtkScalars *fs=vtkScalars::New(); fs->Allocate(1);
  fs->InsertScalar(0, 0.0);
  this->XCoordinates = fs; fs->Register(this);
  this->YCoordinates = fs; fs->Register(this);
  this->ZCoordinates = fs; fs->Register(this);
  fs->Delete();

  this->PointReturn[0] = 0.0;
  this->PointReturn[1] = 0.0;
  this->PointReturn[2] = 0.0;

  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = 0;

}

//----------------------------------------------------------------------------
vtkRectilinearGrid::~vtkRectilinearGrid()
{
  this->Initialize();
  this->Vertex->Delete();
  this->Line->Delete();
  this->Pixel->Delete();
  this->Voxel->Delete();

}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::Initialize()
{
  vtkDataSet::Initialize();

  if ( this->XCoordinates ) 
    {
    this->XCoordinates->Delete();
    this->XCoordinates = NULL;
    }

  if ( this->YCoordinates ) 
    {
    this->YCoordinates->Delete();
    this->YCoordinates = NULL;
    }

  if ( this->ZCoordinates ) 
    {
    this->ZCoordinates->Delete();
    this->ZCoordinates = NULL;
    }
}


//----------------------------------------------------------------------------
vtkRectilinearGrid::vtkRectilinearGrid(const vtkRectilinearGrid& v) :
vtkDataSet(v)
{
  this->Dimensions[0] = v.Dimensions[0];
  this->Dimensions[1] = v.Dimensions[1];
  this->Dimensions[2] = v.Dimensions[2];
  this->DataDescription = v.DataDescription;

  if ( this->XCoordinates != v.XCoordinates )
    {
    this->XCoordinates = v.XCoordinates;
    if ( this->XCoordinates )
      {
      this->XCoordinates->Register(this);
      }
    }

  if ( this->YCoordinates != v.YCoordinates )
    {
    this->YCoordinates = v.YCoordinates;
    if ( this->YCoordinates )
      {
      this->YCoordinates->Register(this);
      }
    }

  if ( this->ZCoordinates != v.ZCoordinates )
    {
    this->ZCoordinates = v.ZCoordinates;
    if ( this->ZCoordinates )
      {
      this->ZCoordinates->Register(this);
      }
    }
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input rectilinear grid
// object.
void vtkRectilinearGrid::CopyStructure(vtkDataSet *ds)
{
  vtkRectilinearGrid *rGrid=(vtkRectilinearGrid *)ds;
  this->Initialize();

  for (int i=0; i<3; i++)
    {
    this->Dimensions[i] = rGrid->Dimensions[i];
    }
  this->DataDescription = rGrid->DataDescription;

  this->XCoordinates = rGrid->XCoordinates;
  if ( this->XCoordinates )
    {
    this->XCoordinates->Register(this);
    }

  this->YCoordinates = rGrid->YCoordinates;
  if ( this->YCoordinates )
    {
    this->YCoordinates->Register(this);
    }

  this->ZCoordinates = rGrid->ZCoordinates;
  if ( this->ZCoordinates )
    {
    this->ZCoordinates->Register(this);
    }
 }

//----------------------------------------------------------------------------
vtkCell *vtkRectilinearGrid::GetCell(int cellId)
{
  vtkCell *cell = NULL;
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
    {
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
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      cell = this->Voxel;
      break;
    }


  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = this->ZCoordinates->GetScalar(loc[2]);
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = this->YCoordinates->GetScalar(loc[1]);
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = this->XCoordinates->GetScalar(loc[0]);

        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::GetCell(int cellId, vtkGenericCell *cell)
{
  int idx, loc[3], npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
    {
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
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      cell->SetCellTypeToVoxel();
      break;
    }

  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = this->ZCoordinates->GetScalar(loc[2]);
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = this->YCoordinates->GetScalar(loc[1]);
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = this->XCoordinates->GetScalar(loc[0]);
        idx = loc[0] + loc[1]*this->Dimensions[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkRectilinearGrid::GetCellBounds(int cellId, float bounds[6])
{
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
    {
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
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      break;
    }

  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;

  // Extract point coordinates
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = this->ZCoordinates->GetScalar(loc[2]);
    bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
    bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
    {
    x[1] = this->YCoordinates->GetScalar(loc[1]);
    bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
    bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
    }
  for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
    {
    x[0] = this->XCoordinates->GetScalar(loc[0]);
    bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
    bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
    }
}


//----------------------------------------------------------------------------
float *vtkRectilinearGrid::GetPoint(int ptId)
{
  int loc[3];
  
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

  this->PointReturn[0] = this->XCoordinates->GetScalar(loc[0]);
  this->PointReturn[1] = this->YCoordinates->GetScalar(loc[1]);
  this->PointReturn[2] = this->ZCoordinates->GetScalar(loc[2]);

  return this->PointReturn;
}

void vtkRectilinearGrid::GetPoint(int ptId, float x[3])
{
  int loc[3];
  
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

  x[0] = this->XCoordinates->GetScalar(loc[0]);
  x[1] = this->YCoordinates->GetScalar(loc[1]);
  x[2] = this->ZCoordinates->GetScalar(loc[2]);

}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::FindPoint(float x[3])
{
  int i, j, loc[3];
  float xPrev, xNext;
  vtkScalars *scalars[3];

  scalars[0] = this->XCoordinates;
  scalars[1] = this->YCoordinates;
  scalars[2] = this->ZCoordinates;
//
// Find coordinates in x-y-z direction
//
  for ( j=0; j < 3; j++ )
    {
    loc[j] = 0;
    xPrev = scalars[j]->GetScalar(0);
    xNext = scalars[j]->GetScalar(scalars[j]->GetNumberOfScalars()-1);
    if ( x[j] < xPrev || x[j] > xNext )
      {
      return -1;
      }

    for (i=1; i < scalars[j]->GetNumberOfScalars(); i++)
      {
      xNext = scalars[j]->GetScalar(i);
      if ( x[j] >= xPrev && x[j] <= xNext )
        {
        if ( (x[j]-xPrev) < (xNext-x[j]) )
	  {
	  loc[j] = i-1;
	  }
        else
	  {
	  loc[j] = i;
	  }
        }
      xPrev = xNext;
      }
    }
//
//  From this location get the point id
//
  return loc[2]*this->Dimensions[0]*this->Dimensions[1] +
         loc[1]*this->Dimensions[0] + loc[0];
  
}
int vtkRectilinearGrid::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
				 vtkGenericCell *vtkNotUsed(gencell),
				 int vtkNotUsed(cellId), 
				 float vtkNotUsed(tol2), 
				 int& subId, float pcoords[3], 
				 float *weights)
{
  return
    this->FindCell( x, (vtkCell *)NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                         int vtkNotUsed(cellId), float vtkNotUsed(tol2), 
                         int& subId, float pcoords[3], float *weights)
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

//----------------------------------------------------------------------------
vtkCell *vtkRectilinearGrid::FindAndGetCell(float x[3], vtkCell *vtkNotUsed(cell), 
                int vtkNotUsed(cellId), float vtkNotUsed(tol2), int& subId, 
                float pcoords[3], float *weights)
{
  int loc[3], cellId;

  subId = 0;
  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return NULL;
    }
  //
  // Get the parametric coordinates and weights for interpolation
  //
  vtkVoxel::InterpolationFunctions(pcoords,weights);
  //
  // Get the cell
  //
  cellId = loc[2] * (this->Dimensions[0]-1)*(this->Dimensions[1]-1) +
           loc[1] * (this->Dimensions[0]-1) + loc[0];

  return vtkRectilinearGrid::GetCell(cellId);
}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::GetCellType(int vtkNotUsed(cellId))
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
      return VTK_EMPTY_CELL;
    }
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::ComputeBounds()
{
  float tmp;

  this->Bounds[0] = this->XCoordinates->GetScalar(0);
  this->Bounds[2] = this->YCoordinates->GetScalar(0);
  this->Bounds[4] = this->ZCoordinates->GetScalar(0);

  this->Bounds[1] = this->XCoordinates->GetScalar(
                        this->XCoordinates->GetNumberOfScalars()-1);
  this->Bounds[3] = this->YCoordinates->GetScalar(
                        this->YCoordinates->GetNumberOfScalars()-1);
  this->Bounds[5] = this->ZCoordinates->GetScalar(
                        this->ZCoordinates->GetNumberOfScalars()-1);
  // ensure that the bounds are increasing
  for (int i = 0; i < 5; i += 2)
    {
    if (this->Bounds[i + 1] < this->Bounds[i])
      {
      tmp = this->Bounds[i + 1];
      this->Bounds[i + 1] = this->Bounds[i];
      this->Bounds[i] = tmp;
      }
    }
}

//----------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkRectilinearGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of rectilinear grid dataset.
void vtkRectilinearGrid::SetDimensions(int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetExtent(int extent[6])
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
  this->Dimensions[0] = extent[1] - extent[0] + 1;
  this->Dimensions[1] = extent[3] - extent[2] + 1;
  this->Dimensions[2] = extent[5] - extent[4] + 1;
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::SetExtent(int xMin, int xMax, int yMin, int yMax,
				   int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  
  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The cell is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the grid, and a 1 if inside the grid.
int vtkRectilinearGrid::ComputeStructuredCoordinates(float x[3], int ijk[3], 
                                                      float pcoords[3])
{
  int i, j;
  float xPrev, xNext, tmp;
  vtkScalars *scalars[3];

  scalars[0] = this->XCoordinates;
  scalars[1] = this->YCoordinates;
  scalars[2] = this->ZCoordinates;
  //
  // Find locations in x-y-z direction
  //
  ijk[0] = ijk[1] = ijk[2] = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;

  for ( j=0; j < 3; j++ )
    {
    xPrev = scalars[j]->GetScalar(0);
    xNext = scalars[j]->GetScalar(scalars[j]->GetNumberOfScalars()-1);
    if (xNext < xPrev)
      {
      tmp = xNext;
      xNext = xPrev;
      xPrev = tmp;
      }
    if ( x[j] < xPrev || x[j] > xNext )
      {
      return 0;
      }

    for (i=1; i < scalars[j]->GetNumberOfScalars(); i++)
      {
      xNext = scalars[j]->GetScalar(i);
      if ( x[j] >= xPrev && x[j] < xNext )
        {
        ijk[j] = i - 1;
        pcoords[j] = (x[j]-xPrev) / (xNext-xPrev);
        break;
        }

      else if ( x[j] == xNext )
        {
        ijk[j] = i - 1;
        pcoords[j] = 1.0;
        break;
        }
      xPrev = xNext;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Should we split up cells, or just points.  It does not matter for now.
// Extent of structured data assumes points.
void vtkRectilinearGrid::SetUpdateExtent(int piece, int numPieces)
{
  int ext[6], zdim, min, max;
  
  // Lets just divide up the z axis.
  this->GetWholeExtent(ext);
  zdim = ext[5] - ext[4] + 1;
  
  if (piece >= zdim)
    {
    // empty
    this->SetUpdateExtent(0, -1, 0, -1, 0, -1);
    return;
    }
  
  if (numPieces > zdim)
    {
    numPieces = zdim;
    }
  
  min = ext[4] + piece * zdim / numPieces;
  max = ext[4] + (piece+1) * zdim / numPieces - 1;
  
  ext[4] = min;
  ext[5] = max;

  this->SetUpdateExtent(ext);
}

//----------------------------------------------------------------------------
unsigned long vtkRectilinearGrid::GetActualMemorySize()
{
  unsigned long size=this->vtkDataSet::GetActualMemorySize();

  if ( this->XCoordinates ) 
    {
    size += this->XCoordinates->GetActualMemorySize();
    }

  if ( this->YCoordinates ) 
    {
    size += this->YCoordinates->GetActualMemorySize();
    }

  if ( this->ZCoordinates ) 
    {
    size += this->ZCoordinates->GetActualMemorySize();
    }

  return size;
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::GetCellNeighbors(int cellId, vtkIdList *ptIds,
                                          vtkIdList *cellIds)
{
  int numPtIds=ptIds->GetNumberOfIds();

  // Use special methods for speed
  switch (numPtIds)
    {
    case 0:
      cellIds->Reset();
      return;

    case 1: case 2: case 4: //vertex, edge, face neighbors
      vtkStructuredData::GetCellNeigbors(cellId, ptIds, 
                                         cellIds, this->Dimensions);
      break;
      
    default:
      this->vtkDataSet::GetCellNeighbors(cellId, ptIds, cellIds);
    }
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "X Coordinates: " << this->XCoordinates << "\n";
  os << indent << "Y Coordinates: " << this->YCoordinates << "\n";
  os << indent << "Z Coordinates: " << this->ZCoordinates << "\n";

  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << endl;

  os << indent << "Extent: " << this->Extent[0] << ", "
     << this->Extent[1] << ", " << this->Extent[2] << ", "
     << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

}


