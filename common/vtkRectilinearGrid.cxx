/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGrid.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkRectilinearGrid.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkVoxel.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"


//----------------------------------------------------------------------------
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

  vtkFloatArray *fs=vtkFloatArray::New(); fs->Allocate(1);
  fs->SetComponent(0, 0, 0.0);
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
// Copy the geometric and topological structure of an input rectilinear grid
// object.
void vtkRectilinearGrid::CopyStructure(vtkDataSet *ds)
{
  vtkRectilinearGrid *rGrid=(vtkRectilinearGrid *)ds;
  int i;
  this->Initialize();

  for (i=0; i<3; i++)
    {
    this->Dimensions[i] = rGrid->Dimensions[i];
    }
  for (i=0; i<6; i++)
    {
    this->Extent[i] = rGrid->Extent[i];
    }
  this->DataDescription = rGrid->DataDescription;

  this->SetXCoordinates(rGrid->XCoordinates);
  this->SetYCoordinates(rGrid->YCoordinates);
  this->SetZCoordinates(rGrid->ZCoordinates);
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
    x[2] = this->ZCoordinates->GetComponent(loc[2], 0);
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = this->YCoordinates->GetComponent(loc[1], 0);
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = this->XCoordinates->GetComponent(loc[0], 0);

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
    x[2] = this->ZCoordinates->GetComponent(loc[2], 0);
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = this->YCoordinates->GetComponent(loc[1], 0);
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = this->XCoordinates->GetComponent(loc[0], 0);
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
    x[2] = this->ZCoordinates->GetComponent(loc[2], 0);
    bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
    bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
    {
    x[1] = this->YCoordinates->GetComponent(loc[1], 0);
    bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
    bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
    }
  for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
    {
    x[0] = this->XCoordinates->GetComponent(loc[0], 0);
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

  this->PointReturn[0] = this->XCoordinates->GetComponent(loc[0], 0);
  this->PointReturn[1] = this->YCoordinates->GetComponent(loc[1], 0);
  this->PointReturn[2] = this->ZCoordinates->GetComponent(loc[2], 0);

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

  x[0] = this->XCoordinates->GetComponent(loc[0], 0);
  x[1] = this->YCoordinates->GetComponent(loc[1], 0);
  x[2] = this->ZCoordinates->GetComponent(loc[2], 0);

}

//----------------------------------------------------------------------------
int vtkRectilinearGrid::FindPoint(float x[3])
{
  int i, j, loc[3];
  float xPrev, xNext;
  vtkDataArray *scalars[3];

  scalars[0] = this->XCoordinates;
  scalars[1] = this->YCoordinates;
  scalars[2] = this->ZCoordinates;
//
// Find coordinates in x-y-z direction
//
  for ( j=0; j < 3; j++ )
    {
    loc[j] = 0;
    xPrev = scalars[j]->GetComponent(0, 0);
    xNext = scalars[j]->GetComponent(scalars[j]->GetNumberOfTuples()-1, 0);
    if ( x[j] < xPrev || x[j] > xNext )
      {
      return -1;
      }

    for (i=1; i < scalars[j]->GetNumberOfTuples(); i++)
      {
      xNext = scalars[j]->GetComponent(i, 0);
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

  if (this->XCoordinates == NULL || this->YCoordinates == NULL || 
      this->ZCoordinates == NULL)
    {
    this->Bounds[0] = this->Bounds[1] = this->Bounds[2] = this->Bounds[3]
      = this->Bounds[4] = this->Bounds[5] = 0.0;
    return;
    }
  
  this->Bounds[0] = this->XCoordinates->GetComponent(0, 0);
  this->Bounds[2] = this->YCoordinates->GetComponent(0, 0);
  this->Bounds[4] = this->ZCoordinates->GetComponent(0, 0);

  this->Bounds[1] = this->XCoordinates->GetComponent(
                        this->XCoordinates->GetNumberOfTuples()-1, 0);
  this->Bounds[3] = this->YCoordinates->GetComponent(
                        this->YCoordinates->GetNumberOfTuples()-1, 0);
  this->Bounds[5] = this->ZCoordinates->GetComponent(
                        this->ZCoordinates->GetNumberOfTuples()-1, 0);
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
  vtkDataArray *scalars[3];

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
    xPrev = scalars[j]->GetComponent(0, 0);
    xNext = scalars[j]->GetComponent(scalars[j]->GetNumberOfTuples()-1, 0);
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

    for (i=1; i < scalars[j]->GetNumberOfTuples(); i++)
      {
      xNext = scalars[j]->GetComponent(i, 0);
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
void vtkRectilinearGrid::SetUpdateExtent(int piece, int numPieces,
					 int ghostLevel)
{
  int ext[6], zdim, min, max, oldZMin, oldZMax;
  
  // Lets just divide up the z axis.
  this->GetWholeExtent(ext);
  zdim = ext[5] - ext[4] + 1;
  
  oldZMin = ext[4];
  oldZMax = ext[5];
  
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

  if (ext[4] - ghostLevel >= oldZMin)
    {
    ext[4] = ext[4] - ghostLevel;
    }
  else
    {
    ext[4] = oldZMin;
    }
  
  if (ext[5] + ghostLevel <= oldZMax)
    {
    ext[5] = ext[5] + ghostLevel;
    }
  else
    {
    ext[5] = oldZMax;
    }
  
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
void vtkRectilinearGrid::ShallowCopy(vtkDataObject *dataObject)
{
  vtkRectilinearGrid *grid = vtkRectilinearGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    this->SetDimensions(grid->GetDimensions());
    this->DataDescription = grid->DataDescription;
    
    this->SetXCoordinates(grid->GetXCoordinates());
    this->SetYCoordinates(grid->GetYCoordinates());
    this->SetZCoordinates(grid->GetZCoordinates());
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkRectilinearGrid::DeepCopy(vtkDataObject *dataObject)
{
  vtkRectilinearGrid *grid = vtkRectilinearGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    vtkFloatArray *s;
    this->SetDimensions(grid->GetDimensions());
    this->DataDescription = grid->DataDescription;
    
    s = vtkFloatArray::New();
    s->DeepCopy(grid->GetXCoordinates());
    this->SetXCoordinates(s);
    s->Delete();
    s = vtkFloatArray::New();
    s->DeepCopy(grid->GetYCoordinates());
    this->SetYCoordinates(s);
    s->Delete();
    s = vtkFloatArray::New();
    s->DeepCopy(grid->GetZCoordinates());
    this->SetZCoordinates(s);
    s->Delete();
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
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


