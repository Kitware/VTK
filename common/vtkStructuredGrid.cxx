/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.cxx
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
#include "vtkStructuredGrid.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkHexahedron.h"
#include "vtkEmptyCell.h"
#include "vtkExtentTranslator.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkStructuredGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredGrid");
  if(ret)
    {
    return (vtkStructuredGrid*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredGrid;
}

#define vtkAdjustBoundsMacro( A, B ) \
  A[0] = (B[0] < A[0] ? B[0] : A[0]);   A[1] = (B[0] > A[1] ? B[0] : A[1]); \
  A[2] = (B[1] < A[2] ? B[1] : A[2]);   A[3] = (B[1] > A[3] ? B[1] : A[3]); \
  A[4] = (B[2] < A[4] ? B[2] : A[4]);   A[5] = (B[2] > A[5] ? B[2] : A[5])

vtkStructuredGrid::vtkStructuredGrid()
{
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Hexahedron = vtkHexahedron::New();
  
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;
  
  this->Blanking = 0;
  this->PointVisibility = NULL;

  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = 0;
  
}

//----------------------------------------------------------------------------
vtkStructuredGrid::~vtkStructuredGrid()
{
  this->Initialize();

  this->Vertex->Delete();
  this->Line->Delete();
  this->Quad->Delete();
  this->Hexahedron->Delete();
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured grid.
void vtkStructuredGrid::CopyStructure(vtkDataSet *ds)
{
  vtkStructuredGrid *sg=(vtkStructuredGrid *)ds;
  vtkPointSet::CopyStructure(ds);
  int i;

  for (i=0; i<3; i++)
    {
    this->Dimensions[i] = sg->Dimensions[i];
    }
  for (i=0; i<6; i++)
    {
    this->Extent[i] = sg->Extent[i];
    }

  this->DataDescription = sg->DataDescription;

  this->Blanking = sg->Blanking;
  this->SetPointVisibility(sg->PointVisibility);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::Initialize()
{
  vtkPointSet::Initialize(); 
  if ( this->PointVisibility ) 
    {
    this->PointVisibility->UnRegister(this);
    }
  this->PointVisibility = NULL;
  this->Blanking = 0;
}

//----------------------------------------------------------------------------
int vtkStructuredGrid::GetCellType(vtkIdType cellId)
{
  // see whether the cell is blanked
  if ( this->Blanking && !this->IsCellVisible(cellId) )
    {
    return VTK_EMPTY_CELL;
    }

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: 
      return VTK_VERTEX;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE:
      return VTK_LINE;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE:
      return VTK_QUAD;

    case VTK_XYZ_GRID:
      return VTK_HEXAHEDRON;

    default:
      vtkErrorMacro(<<"Bad data description!");
      return VTK_EMPTY_CELL;
    }
}

//----------------------------------------------------------------------------
vtkCell *vtkStructuredGrid::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  vtkIdType idx;
  int i, j, k;
  int d01, offset1, offset2;
 
  // Make sure data is defined
  if ( ! this->Points )
    {
    vtkErrorMacro (<<"No data");
    return NULL;
    }
 
  // see whether the cell is blanked
  if ( this->Blanking && !this->IsCellVisible(cellId) )
    {
    return this->EmptyCell;
    }

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      cell->PointIds->SetId(0,0);
      break;

    case VTK_X_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_Y_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_Z_LINE:
      cell = this->Line;
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_XY_PLANE:
      cell = this->Quad;
      i = cellId % (this->Dimensions[0]-1);
      j = cellId / (this->Dimensions[0]-1);
      idx = i + j*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_YZ_PLANE:
      cell = this->Quad;
      j = cellId % (this->Dimensions[1]-1);
      k = cellId / (this->Dimensions[1]-1);
      idx = j + k*this->Dimensions[1];
      offset1 = 1;
      offset2 = this->Dimensions[1];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_XZ_PLANE:
      cell = this->Quad;
      i = cellId % (this->Dimensions[0]-1);
      k = cellId / (this->Dimensions[0]-1);
      idx = i + k*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_XYZ_GRID:
      cell = this->Hexahedron;
      d01 = this->Dimensions[0]*this->Dimensions[1];
      i = cellId % (this->Dimensions[0] - 1);
      j = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      k = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      idx = i+ j*this->Dimensions[0] + k*d01;
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      idx += d01;
      cell->PointIds->SetId(4,idx);
      cell->PointIds->SetId(5,idx+offset1);
      cell->PointIds->SetId(6,idx+offset1+offset2);
      cell->PointIds->SetId(7,idx+offset2);
      break;
    }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds->GetNumberOfIds();
  for (i=0; i<NumberOfIds; i++)
    {
    idx = cell->PointIds->GetId(i);
    cell->Points->SetPoint(i,this->Points->GetPoint(idx));
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType   idx;
  int   i, j, k;
  int   d01, offset1, offset2;
  float x[3];
 
  // Make sure data is defined
  if ( ! this->Points )
    {
    vtkErrorMacro (<<"No data");
    }
 
  // see whether the cell is blanked
  if ( this->Blanking && !this->IsCellVisible(cellId) )
    {
    cell->SetCellTypeToEmptyCell();
    return;
    }

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell->SetCellTypeToVertex();
      cell->PointIds->SetId(0,0);
      break;

    case VTK_X_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_Y_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_Z_LINE:
      cell->SetCellTypeToLine();
      cell->PointIds->SetId(0,cellId);
      cell->PointIds->SetId(1,cellId+1);
      break;

    case VTK_XY_PLANE:
      cell->SetCellTypeToQuad();
      i = cellId % (this->Dimensions[0]-1);
      j = cellId / (this->Dimensions[0]-1);
      idx = i + j*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_YZ_PLANE:
      cell->SetCellTypeToQuad();
      j = cellId % (this->Dimensions[1]-1);
      k = cellId / (this->Dimensions[1]-1);
      idx = j + k*this->Dimensions[1];
      offset1 = 1;
      offset2 = this->Dimensions[1];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_XZ_PLANE:
      cell->SetCellTypeToQuad();
      i = cellId % (this->Dimensions[0]-1);
      k = cellId / (this->Dimensions[0]-1);
      idx = i + k*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      break;

    case VTK_XYZ_GRID:
      cell->SetCellTypeToHexahedron();
      d01 = this->Dimensions[0]*this->Dimensions[1];
      i = cellId % (this->Dimensions[0] - 1);
      j = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      k = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      idx = i+ j*this->Dimensions[0] + k*d01;
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds->SetId(0,idx);
      cell->PointIds->SetId(1,idx+offset1);
      cell->PointIds->SetId(2,idx+offset1+offset2);
      cell->PointIds->SetId(3,idx+offset2);
      idx += d01;
      cell->PointIds->SetId(4,idx);
      cell->PointIds->SetId(5,idx+offset1);
      cell->PointIds->SetId(6,idx+offset1+offset2);
      cell->PointIds->SetId(7,idx+offset2);
      break;
    }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds->GetNumberOfIds();
  for (i=0; i<NumberOfIds; i++)
    {
    idx = cell->PointIds->GetId(i);
    this->Points->GetPoint(idx, x);
    cell->Points->SetPoint(i, x);
    }
}


//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkStructuredGrid::GetCellBounds(vtkIdType cellId, float bounds[6])
{
  vtkIdType idx = 0;
  int i, j, k;
  vtkIdType d01;
  int offset1 = 0;
  int offset2 = 0;
  float x[3];
  
  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
  
  // Make sure data is defined
  if ( ! this->Points )
    {
    vtkErrorMacro (<<"No data");
    return;
    }
 
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      this->Points->GetPoint( 0, x );
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];
      break;

    case VTK_X_LINE:
    case VTK_Y_LINE:
    case VTK_Z_LINE:
      this->Points->GetPoint( cellId, x );
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint( cellId +1, x );
      vtkAdjustBoundsMacro( bounds, x );
      break;

    case VTK_XY_PLANE:
    case VTK_YZ_PLANE:
    case VTK_XZ_PLANE:
      if (this->DataDescription == VTK_XY_PLANE)
        {
        i = cellId % (this->Dimensions[0]-1);
        j = cellId / (this->Dimensions[0]-1);
        idx = i + j*this->Dimensions[0];
        offset1 = 1;
        offset2 = this->Dimensions[0];
        }
      else if (this->DataDescription == VTK_YZ_PLANE)
        {
        j = cellId % (this->Dimensions[1]-1);
        k = cellId / (this->Dimensions[1]-1);
        idx = j + k*this->Dimensions[1];
        offset1 = 1;
        offset2 = this->Dimensions[1];
        }
      else if (this->DataDescription == VTK_XZ_PLANE)
        {
        i = cellId % (this->Dimensions[0]-1);
        k = cellId / (this->Dimensions[0]-1);
        idx = i + k*this->Dimensions[0];
        offset1 = 1;
        offset2 = this->Dimensions[0];
        }

      this->Points->GetPoint(idx, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint( idx+offset1, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset1+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      break;

    case VTK_XYZ_GRID:
      d01 = this->Dimensions[0]*this->Dimensions[1];
      i = cellId % (this->Dimensions[0] - 1);
      j = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      k = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      idx = i+ j*this->Dimensions[0] + k*d01;
      offset1 = 1;
      offset2 = this->Dimensions[0];

      this->Points->GetPoint(idx, x);
      bounds[0] = bounds[1] = x[0];
      bounds[2] = bounds[3] = x[1];
      bounds[4] = bounds[5] = x[2];

      this->Points->GetPoint( idx+offset1, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset1+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      idx += d01;

      this->Points->GetPoint(idx, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset1, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset1+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      this->Points->GetPoint( idx+offset2, x);
      vtkAdjustBoundsMacro( bounds, x );

      break;
    }
}


//----------------------------------------------------------------------------
// Turn on data blanking. Data blanking is the ability to turn off
// portions of the grid when displaying or operating on it. Some data
// (like finite difference data) routinely turns off data to simulate
// solid obstacles.
void vtkStructuredGrid::BlankingOn()
{
  if (!this->Blanking)
    {
    this->Blanking = 1;
    this->Modified();

    if ( !this->PointVisibility )
      {
      this->AllocatePointVisibility();
      }
    }
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::AllocatePointVisibility()
{
  if ( !this->PointVisibility )
    {
    this->PointVisibility = vtkUnsignedCharArray::New();
    this->PointVisibility->Allocate(this->GetNumberOfPoints(),1000);
    for (int i=0; i<this->GetNumberOfPoints(); i++)
      {
      this->PointVisibility->SetValue(i,1);
      }
    }
}

//----------------------------------------------------------------------------
// Turn off data blanking.
void vtkStructuredGrid::BlankingOff()
{
  if (this->Blanking)
    {
    this->Blanking = 0;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Turn off data blanking.
void vtkStructuredGrid::SetBlanking(int b)
{
  if ( b )
    {
    this->BlankingOn();
    }
  else
    {
    this->BlankingOff();
    }
}

//----------------------------------------------------------------------------
// Turn off a particular data point.
void vtkStructuredGrid::BlankPoint(vtkIdType ptId)
{
  if ( !this->PointVisibility )
    {
    this->AllocatePointVisibility();
    }
  this->PointVisibility->SetValue(ptId,0);
}

//----------------------------------------------------------------------------
// Turn on a particular data point.
void vtkStructuredGrid::UnBlankPoint(vtkIdType ptId)
{
  if ( !this->PointVisibility )
    {
    this->AllocatePointVisibility();
    }
  this->PointVisibility->SetValue(ptId,1);
}

void vtkStructuredGrid::SetPointVisibility(vtkUnsignedCharArray *ptVis)
{
  if ( ptVis != this->PointVisibility )
    {
    if ( this->PointVisibility )
      {
      this->PointVisibility->Delete();
      }
    this->PointVisibility = ptVis;
    if ( ptVis )
      {
      ptVis->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkStructuredGrid::IsCellVisible(vtkIdType cellId)
{
  int numIds=0;
  vtkIdType ptIds[8];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  vtkIdType d01 = this->Dimensions[0]*this->Dimensions[1];
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      numIds = 1;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      numIds = 2;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMax + jMin*this->Dimensions[0] + kMin*d01;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      numIds = 2;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMin + jMax*this->Dimensions[0] + kMin*d01;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      numIds = 2;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMin + jMin*this->Dimensions[0] + kMax*d01;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      numIds = 4;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMax + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[2] = iMax + jMax*this->Dimensions[0] + kMin*d01;
      ptIds[3] = iMin + jMax*this->Dimensions[0] + kMin*d01;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      numIds = 4;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMin + jMax*this->Dimensions[0] + kMin*d01;
      ptIds[2] = iMin + jMax*this->Dimensions[0] + kMax*d01;
      ptIds[3] = iMin + jMin*this->Dimensions[0] + kMax*d01;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      numIds = 4;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMax + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[2] = iMax + jMin*this->Dimensions[0] + kMax*d01;
      ptIds[3] = iMin + jMin*this->Dimensions[0] + kMax*d01;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      numIds = 8;
      ptIds[0] = iMin + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[1] = iMax + jMin*this->Dimensions[0] + kMin*d01;
      ptIds[2] = iMax + jMax*this->Dimensions[0] + kMin*d01;
      ptIds[3] = iMin + jMax*this->Dimensions[0] + kMin*d01;
      ptIds[4] = iMin + jMin*this->Dimensions[0] + kMax*d01;
      ptIds[5] = iMax + jMin*this->Dimensions[0] + kMax*d01;
      ptIds[6] = iMax + jMax*this->Dimensions[0] + kMax*d01;
      ptIds[7] = iMin + jMax*this->Dimensions[0] + kMax*d01;
      break;
    }

  for (int i=0; i<numIds; i++)
    {
    if ( !this->IsPointVisible(ptIds[i]) )
      {
      return 0;
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}

//----------------------------------------------------------------------------
// Get the points defining a cell. (See vtkDataSet for more info.)
void vtkStructuredGrid::GetCellPoints(vtkIdType cellId, vtkIdList *ptIds)
{
  int iMin, iMax, jMin, jMax, kMin, kMax;
  vtkIdType d01 = this->Dimensions[0]*this->Dimensions[1];
 
  ptIds->Reset();
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      ptIds->SetNumberOfIds(1);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMax + jMin*this->Dimensions[0] + kMin*d01);
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMin + jMax*this->Dimensions[0] + kMin*d01);
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      ptIds->SetNumberOfIds(2);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMin + jMin*this->Dimensions[0] + kMax*d01);
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (this->Dimensions[0]-1);
      jMax = jMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMax + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(2, iMax + jMax*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(3, iMin + jMax*this->Dimensions[0] + kMin*d01);
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (this->Dimensions[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (this->Dimensions[1]-1);
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMin + jMax*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(2, iMin + jMax*this->Dimensions[0] + kMax*d01);
      ptIds->SetId(3, iMin + jMin*this->Dimensions[0] + kMax*d01);
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (this->Dimensions[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (this->Dimensions[0]-1);
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(4);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMax + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(2, iMax + jMin*this->Dimensions[0] + kMax*d01);
      ptIds->SetId(3, iMin + jMin*this->Dimensions[0] + kMax*d01);
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (this->Dimensions[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      kMax = kMin + 1;
      ptIds->SetNumberOfIds(8);
      ptIds->SetId(0, iMin + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(1, iMax + jMin*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(2, iMax + jMax*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(3, iMin + jMax*this->Dimensions[0] + kMin*d01);
      ptIds->SetId(4, iMin + jMin*this->Dimensions[0] + kMax*d01);
      ptIds->SetId(5, iMax + jMin*this->Dimensions[0] + kMax*d01);
      ptIds->SetId(6, iMax + jMax*this->Dimensions[0] + kMax*d01);
      ptIds->SetId(7, iMin + jMax*this->Dimensions[0] + kMax*d01);
      break;
    }
}

//----------------------------------------------------------------------------
// Should we split up cells, or just points.  It does not matter for now.
// Extent of structured data assumes points.
void vtkStructuredGrid::SetUpdateExtent(int piece, int numPieces,
					int ghostLevel)
{
  int ext[6];
  
  this->UpdateInformation();
  this->GetWholeExtent(ext);
  this->ExtentTranslator->SetWholeExtent(ext);
  this->ExtentTranslator->SetPiece(piece);
  this->ExtentTranslator->SetNumberOfPieces(numPieces);
  this->ExtentTranslator->SetGhostLevel(ghostLevel);
  this->ExtentTranslator->PieceToExtent();
  this->SetUpdateExtent(this->ExtentTranslator->GetExtent());

  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetExtent(int extent[6])
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
void vtkStructuredGrid::SetExtent(int xMin, int xMax, 
				  int yMin, int yMax,
				  int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  
  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
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
  
  // If blanking, remove blanked cells.
  if ( this->Blanking )
    {
    int xcellId;
    for (int i=0; i<cellIds->GetNumberOfIds(); i++)
      {
      xcellId = cellIds->GetId(i);
      if ( !this->IsCellVisible(xcellId) )
        {
        cellIds->DeleteId(xcellId);
        }
      }
    }
}

//----------------------------------------------------------------------------
unsigned long vtkStructuredGrid::GetActualMemorySize()
{
  return this->vtkPointSet::GetActualMemorySize();
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::ShallowCopy(vtkDataObject *dataObject)
{
  vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    this->InternalStructuredGridCopy(grid);
    }

  // Do superclass
  this->vtkPointSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::DeepCopy(vtkDataObject *dataObject)
{
  vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(dataObject);

  if ( grid != NULL )
    {
    this->InternalStructuredGridCopy(grid);
    }

  // Do superclass
  this->vtkPointSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkStructuredGrid::InternalStructuredGridCopy(vtkStructuredGrid *src)
{
  int idx;

  this->DataDescription = src->DataDescription;
  this->Blanking = src->Blanking;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    }
}

//----------------------------------------------------------------------------
// Override this method because of blanking
void vtkStructuredGrid::GetScalarRange(float range[2])
{
  vtkScalars *ptScalars = this->PointData->GetScalars();
  vtkScalars *cellScalars = this->CellData->GetScalars();
  float ptRange[2];
  float cellRange[2];
  float s;
  int id, num;
  
  ptRange[0] =  VTK_LARGE_FLOAT;
  ptRange[1] =  -VTK_LARGE_FLOAT;
  if ( ptScalars )
    {
    num = this->GetNumberOfPoints();
    for (id=0; id < num; id++)
      {
      if ( this->IsPointVisible(id) )
        {
        s = ptScalars->GetScalar(id);
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
        s = cellScalars->GetScalar(id);
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

  range[0] = (cellRange[0] >= VTK_LARGE_FLOAT ? 0.0 : cellRange[0]);
  range[1] = (cellRange[1] <= -VTK_LARGE_FLOAT ? 1.0 : cellRange[1]);

  this->ComputeTime.Modified();
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "Blanking: " << (this->Blanking ? "On\n" : "Off\n");

  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", "
     << this->WholeExtent[1] << ", " << this->WholeExtent[2] << ", "
     << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << endl;
  os << indent << "Extent: " << this->Extent[0] << ", "
     << this->Extent[1] << ", " << this->Extent[2] << ", "
     << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << ")\n";
}




