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
#include "vtkStructuredExtent.h"
#include "vtkStructuredInformation.h"

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
  
  // Delete the default object created by the superclass, and
  // create data specific objects.
  this->Information->Delete();
  this->Information = vtkStructuredInformation::New();

  this->UpdateExtent->Delete();
  this->UpdateExtent = vtkStructuredExtent::New();
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
  this->GetStructuredUpdateExtent()->SetExtent(extent);
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
  return this->GetStructuredUpdateExtent()->GetExtent();
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetUpdateExtent(int ext[6])
{
  int *tmp = this->GetStructuredUpdateExtent()->GetExtent();
  
  ext[0] = tmp[0];
  ext[1] = tmp[1];
  ext[2] = tmp[2];
  ext[3] = tmp[3];
  ext[4] = tmp[4];
  ext[5] = tmp[5];
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::SetUpdateExtentToWholeExtent()
{
  vtkStructuredInformation *info = this->GetStructuredInformation();
  
  this->UpdateInformation();
  this->SetUpdateExtent(info->GetWholeExtent());
}


//----------------------------------------------------------------------------
int vtkStructuredGrid::ClipUpdateExtentWithWholeExtent()
{
  int *wholeExt = this->GetStructuredInformation()->GetWholeExtent();
  int valid = 1;
  int idx, minIdx, maxIdx;
  int uExt[6];
  
  this->GetStructuredUpdateExtent()->GetExtent(uExt);
  
  for (idx = 0; idx < 3; ++idx)
    {
    minIdx = 2*idx;
    maxIdx = 2*idx + 1;
    
    // make sure there is overlap!
    if (uExt[minIdx] > wholeExt[maxIdx])
      {
      valid = 0;
      vtkErrorMacro("UpdateExtent " << uExt[minIdx] 
		      << " -> " << uExt[maxIdx]  
		      << " does not overlap with WholeExtent "
		      << wholeExt[minIdx] << " -> "
		      << wholeExt[maxIdx]);
      uExt[minIdx] = wholeExt[maxIdx];
      }
    if (uExt[maxIdx] < wholeExt[minIdx])
      {
      valid = 0;
      vtkErrorMacro("UpdateExtent " << uExt[minIdx] 
		      << " -> " << uExt[maxIdx]  
		      << " does not overlap with WholeExtent "
		      << wholeExt[minIdx] << " -> "
		      << wholeExt[maxIdx]);
      uExt[maxIdx] = wholeExt[minIdx];
      }
    
    // typical intersection shift min up to whole min
    if (uExt[minIdx] < wholeExt[minIdx])
      {
      uExt[minIdx] = wholeExt[minIdx];
      }
    // typical intersection shift max down to whole max
    if (uExt[maxIdx] >= wholeExt[maxIdx])
      {
      uExt[maxIdx] = wholeExt[maxIdx];
      }
    }
  
  // Now check to see if the UpdateExtent is in the current Extent.
  
  // If the requested extent is not completely in the data structure already,
  // release the data to cause the source to execute.
  if (this->Extent[0] > uExt[0] || 
      this->Extent[1] < uExt[1] || 
      this->Extent[2] > uExt[2] || 
      this->Extent[3] < uExt[3] ||
      this->Extent[4] > uExt[4] ||
      this->Extent[5] < uExt[5])
    {
    this->ReleaseData();
    }
  
  this->GetStructuredUpdateExtent()->SetExtent(uExt);
  
  return valid;
}

//----------------------------------------------------------------------------
// Should we split up cells, or just points.  It does not matter for now.
// Extent of structured data assumes points.
void vtkStructuredGrid::SetUpdateExtent(int piece, int numPieces)
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
void vtkStructuredGrid::SetWholeExtent(int extent[6])
{
  this->GetStructuredInformation()->SetWholeExtent(extent);
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
int *vtkStructuredGrid::GetWholeExtent() 
{
  return this->GetStructuredInformation()->GetWholeExtent();
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::GetWholeExtent(int extent[6])
{
  int *ext = this->GetStructuredInformation()->GetWholeExtent();
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = ext[idx];
    }
}
//----------------------------------------------------------------------------
void vtkStructuredGrid::GetWholeExtent(int &xMin, int &xMax, 
				       int &yMin, int &yMax,
				       int &zMin, int &zMax)
{
  int *ext = this->GetStructuredInformation()->GetWholeExtent();

  xMin = ext[0];  xMax = ext[1];
  yMin = ext[2];  yMax = ext[3];
  zMin = ext[4];  zMax = ext[5];
}


//----------------------------------------------------------------------------
unsigned long vtkStructuredGrid::GetEstimatedUpdateMemorySize()
{
  int *wExt = this->GetStructuredInformation()->GetWholeExtent();
  int idx, *uExt = this->GetStructuredUpdateExtent()->GetExtent();
  unsigned long wholeSize, updateSize;
  
  // Compute the sizes
  wholeSize = updateSize = 1;
  for (idx = 0; idx < 3; ++idx)
    {
    wholeSize *= (wExt[idx*2+1] - wExt[idx*2] + 1);
    updateSize *= (uExt[idx*2+1] - uExt[idx*2] +1);
    }

  updateSize = updateSize * this->Information->GetEstimatedWholeMemorySize() 
    / wholeSize;
  if (updateSize < 1)
    {
    return 1;
    }
  return updateSize;
}

//----------------------------------------------------------------------------
unsigned long vtkStructuredGrid::GetActualMemorySize()
{
  return this->vtkPointSet::GetActualMemorySize();
}


//----------------------------------------------------------------------------
void vtkStructuredGrid::InternalUpdate()
{
  int *uExt, *ext;

  this->vtkDataObject::InternalUpdate();
  uExt = this->GetUpdateExtent();
  ext = this->GetExtent();
  if (uExt[0] > ext[0] || uExt[1] < ext[1] || uExt[2] > ext[2] || 
      uExt[3] < ext[3] || uExt[4] > ext[4] || uExt[5] < ext[5])
    {
    // vtkWarningMacro("Extent does not match UpdateExtent.");
    // clip the grid
    vtkPointData *inPd = this->GetPointData();
    vtkPointData *newPd = vtkPointData::New();
    int i, j, k, idx, newIdx, yInc, zInc;
    int newSize, jOffset, kOffset;
    vtkPoints *newPts, *inPts;

    inPts = this->GetPoints();

    yInc = (ext[1]-ext[0]+1);
    zInc = yInc * (ext[3]-ext[2]+1) ;
    newSize = (uExt[1]-uExt[0]+1)*(uExt[3]-uExt[2]+1)*(uExt[5]-uExt[4]+1);
    newPts = (vtkPoints *) inPts->MakeObject(); newPts->SetNumberOfPoints(newSize);
    newPd->CopyAllocate(inPd,newSize,newSize);

    //
    // Traverse input data and copy point attributes to newPd
    //
    newIdx = 0;
    for ( k=uExt[4]; k <= uExt[5]; ++k )
      {
      kOffset = k * zInc;
      for ( j=uExt[2]; j <= uExt[3]; ++j )
        {
        jOffset = j * yInc;
        for ( i=uExt[0]; i <= uExt[1]; ++i )
          {
          idx = i + jOffset + kOffset;
          newPts->SetPoint(newIdx,inPts->GetPoint(idx));
          newPd->CopyData(inPd, idx, newIdx++);
          }
        }
      }

    this->SetExtent(uExt);
    this->SetPoints(newPts);
    newPts->Delete();
    this->PointData->Delete();
    this->PointData = newPd;
    }
}

//----------------------------------------------------------------------------
void vtkStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "Blanking: " << (this->Blanking ? "On\n" : "Off\n");

  os << indent << "Extent: " << this->Extent[0] << ", "
     << this->Extent[1] << ", " << this->Extent[2] << ", "
     << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << endl;

  os << indent << "MemoryLimit: " << this->MemoryLimit << endl;

  os << ")\n";
}




