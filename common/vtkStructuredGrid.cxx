/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.cxx
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
#include "vtkStructuredGrid.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkHexahedron.h"

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

  // -----------
  // I do not like this method of defaulting to the whole dimensions.
  this->UpdateExtent[0] = this->UpdateExtent[2] 
    = this->UpdateExtent[4] = -VTK_LARGE_INTEGER;
  this->UpdateExtent[1] = this->UpdateExtent[3] 
    = this->UpdateExtent[5] = VTK_LARGE_INTEGER;

  this->Extent[0] = this->Extent[2] = this->Extent[4] = 0;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = 0;

  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
vtkStructuredGrid::vtkStructuredGrid(const vtkStructuredGrid& sg) :
vtkPointSet(sg)
{
  this->Dimensions[0] = sg.Dimensions[0];
  this->Dimensions[1] = sg.Dimensions[1];
  this->Dimensions[2] = sg.Dimensions[2];
  this->DataDescription = sg.DataDescription;

  this->Blanking = sg.Blanking;
  if ( sg.PointVisibility != NULL )
    {
    this->PointVisibility->UnRegister((vtkObject *)this);
    this->PointVisibility = sg.PointVisibility;
    this->PointVisibility->Register((vtkObject *)this);
    }
  else
    {
    this->PointVisibility = NULL;
    }
}

//----------------------------------------------------------------------------
vtkStructuredGrid::~vtkStructuredGrid()
{
  this->Initialize();
  if (this->PointVisibility) 
    {
    this->PointVisibility->UnRegister((vtkObject *)this);
    }
  this->PointVisibility = NULL;

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
  if ( sg->PointVisibility != NULL && 
  sg->PointVisibility != this->PointVisibility )
    {
    if ( this->PointVisibility ) 
      {
      this->PointVisibility->UnRegister((vtkObject *)this);
      }
    this->PointVisibility = sg->PointVisibility;
    this->PointVisibility->Register((vtkObject *)this);
    }
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
int vtkStructuredGrid::GetCellType(int vtkNotUsed(cellId))
{
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
vtkCell *vtkStructuredGrid::GetCell(int cellId)
{
  vtkCell *cell = NULL;
  int idx;
  int i, j, k;
  int d01, offset1, offset2;
 
  // Make sure data is defined
  if ( ! this->Points )
    {
    vtkErrorMacro (<<"No data");
    return NULL;
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
void vtkStructuredGrid::GetCell(int cellId, vtkGenericCell *cell)
{
  int   idx;
  int   i, j, k;
  int   d01, offset1, offset2;
  float x[3];
 
  // Make sure data is defined
  if ( ! this->Points )
    {
    vtkErrorMacro (<<"No data");
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
void vtkStructuredGrid::GetCellBounds(int cellId, float bounds[6])
{
  int idx;
  int i, j, k;
  int d01, offset1, offset2;
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
    this->PointVisibility = vtkScalars::New(VTK_BIT,1);
    this->PointVisibility->Allocate(this->GetNumberOfPoints(),1000);
    this->PointVisibility->Register((vtkObject *)this);
    for (int i=0; i<this->GetNumberOfPoints(); i++)
      {
      this->PointVisibility->InsertScalar(i,1);
      }
    this->PointVisibility->Delete();
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
// Turn off a particular data point.
void vtkStructuredGrid::BlankPoint(int ptId)
{
  if ( !this->PointVisibility )
    {
    this->AllocatePointVisibility();
    }
  this->PointVisibility->InsertScalar(ptId,0);
}

//----------------------------------------------------------------------------
// Turn on a particular data point.
void vtkStructuredGrid::UnBlankPoint(int ptId)
{
  if ( !this->PointVisibility )
    {
    this->AllocatePointVisibility();
    }
  this->PointVisibility->InsertScalar(ptId,1);
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
void vtkStructuredGrid::GetCellPoints(int cellId, vtkIdList *ptIds)
{
  int idx, loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int d01 = this->Dimensions[0]*this->Dimensions[1];
 
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
//============================= streaming stuff ==============================

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
void vtkStructuredGrid::SetExtent(int xMin, int xMax, int yMin, int yMax,
				                          int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  
  this->SetExtent(extent);
}

//----------------------------------------------------------------------------
int *vtkStructuredGrid::GetExtent()
{
  return this->Extent;
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetUpdateExtent(int extent[6])
{
  int idx;
  
  // If the requested extent is not completely in the data structure already,
  // modify the data structure to cause the source to execute.
  if (this->Extent[0] > extent[0] || this->Extent[1] < extent[1] || 
      this->Extent[2] > extent[2] || this->Extent[3] < extent[3] ||
      this->Extent[4] > extent[4] || this->Extent[5] < extent[5])
    {
    this->ReleaseData();
    }
  for (idx = 0; idx < 6; ++idx)
    {
    this->UpdateExtent[idx] = extent[idx];
    }
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetUpdateExtent(int xMin, int xMax, int yMin, int yMax,
				    int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  
  this->SetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
int *vtkStructuredGrid::GetUpdateExtent()
{
  return this->UpdateExtent;
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetUpdateExtent(int ext[6])
{
  ext[0] = this->UpdateExtent[0];
  ext[1] = this->UpdateExtent[1];
  ext[2] = this->UpdateExtent[2];
  ext[3] = this->UpdateExtent[3];
  ext[4] = this->UpdateExtent[4];
  ext[5] = this->UpdateExtent[5];
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetUpdateExtentToWholeExtent()
{
  this->UpdateInformation();
  this->SetUpdateExtent(this->WholeExtent);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::CopyUpdateExtent(vtkDataObject *data)
{
  vtkStructuredGrid *grid = (vtkStructuredGrid*)(data);
  int *ext;

  if (data->GetDataObjectType() != VTK_STRUCTURED_GRID)
    {
    vtkErrorMacro("CopyUpdateExtent: Expecting vtkStructuredGrid");
    return;
    }

  ext = grid->GetUpdateExtent();
  this->SetUpdateExtent(ext);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::CopyInformation(vtkDataObject *data)
{
  vtkStructuredGrid *grid = (vtkStructuredGrid*)(data);

  // bad .... bad ... bad ...
  if (strcmp(data->GetClassName(), "vtkStructuredGrid") != 0)
    {
    vtkErrorMacro("CopyInformation: Expecting vtkStructuredGrid, got " 
                  << data->GetClassName());
    return;
    }

  this->SetWholeExtent(grid->GetWholeExtent());
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::ClipUpdateExtentWithWholeExtent()
{
  if (this->UpdateExtent[0] < this->WholeExtent[0])
    {
    this->UpdateExtent[0] = this->WholeExtent[0];
    }
  if (this->UpdateExtent[2] < this->WholeExtent[2])
    {
    this->UpdateExtent[2] =  this->WholeExtent[2];
    }
  if (this->UpdateExtent[4] < this->WholeExtent[4])
    {
    this->UpdateExtent[4] =  this->WholeExtent[4];
    }
  if (this->UpdateExtent[1] >= this->WholeExtent[1])
    {
    this->UpdateExtent[1] = this->WholeExtent[1];
    }
  if (this->UpdateExtent[3] >= this->WholeExtent[3])
    {
    this->UpdateExtent[3] = this->WholeExtent[3];
    }
  if (this->UpdateExtent[5] >= this->WholeExtent[5])
    {
    this->UpdateExtent[5] = this->WholeExtent[5];
    }
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetUpdateExtent(int piece, int numPieces)
{
  int *wholeDim;
  int numPiecesInFirstHalf;
  int size[3], mid, splitAxis;
  int ext[6];

  this->UpdateInformation();

  // Start with the whole grid.
  this->GetWholeExtent(ext);  

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext. 
  while (numPieces > 1)
    {
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    if (size[0] > size[1] && size[0] > size[2])
      {
      splitAxis = 0;
      }
    else if (size[1] > size[2])
      {
      splitAxis = 1;
      }
    else if (size[2] > 1)
      {
      splitAxis = 2;
      }
    else
      {
      // signal no more splits possible
      splitAxis = -1;
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty how do we handle this?
        numPieces = 1;
        ext[0] = ext[2] = ext[4] = 0;
        ext[1] = ext[3] = ext[5] = -1;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      mid = (size[splitAxis] / 2) + ext[splitAxis*2];
      numPiecesInFirstHalf = (numPieces / 2);
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid;
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while


  // now transfer to UpdateExtent.
  this->UpdateExtent[0] = ext[0];
  this->UpdateExtent[1] = ext[1];
  this->UpdateExtent[2] = ext[2];
  this->UpdateExtent[3] = ext[3];
  this->UpdateExtent[4] = ext[4];
  this->UpdateExtent[5] = ext[5];
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetWholeExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != extent[idx])
      {
      modified = 1;
      this->WholeExtent[idx] = extent[idx];
      }
    }
  if (modified)
    {
    this->Modified();
    }
}
//----------------------------------------------------------------------------
void vtkStructuredGrid::SetWholeExtent(int xMin, int xMax,
				   int yMin, int yMax, int zMin, int zMax)
{
  int extent[6];

  extent[0] = xMin; extent[1] = xMax;
  extent[2] = yMin; extent[3] = yMax;
  extent[4] = zMin; extent[5] = zMax;
  this->SetWholeExtent(extent);
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetWholeExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->WholeExtent[idx];
    }
}
//----------------------------------------------------------------------------
void vtkStructuredGrid::GetWholeExtent(int &xMin, int &xMax, 
				       int &yMin, int &yMax,
				       int &zMin, int &zMax)
{
  xMin = this->WholeExtent[0]; xMax = this->WholeExtent[1];
  yMin = this->WholeExtent[2]; yMax = this->WholeExtent[3];
  zMin = this->WholeExtent[4]; zMax = this->WholeExtent[5];
}


//----------------------------------------------------------------------------
void vtkStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);
  int idx;

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "Blanking: " << (this->Blanking ? "On\n" : "Off\n");

  os << indent << "UpdateExtent: " << this->UpdateExtent[0] << ", "
     << this->UpdateExtent[1] << ", " << this->UpdateExtent[2] << ", "
     << this->UpdateExtent[3] << ", " << this->UpdateExtent[4] << ", "
     << this->UpdateExtent[5] << endl;

  os << indent << "Extent: " << this->Extent[0] << ", "
     << this->Extent[1] << ", " << this->Extent[2] << ", "
     << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << indent << "EstimatedMemorySize: " << this->EstimatedMemorySize << endl;
  os << indent << "MemoryLimit: " << this->MemoryLimit << endl;

  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
}




