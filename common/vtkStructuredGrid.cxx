/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGrid.cxx
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
#include "vtkStructuredGrid.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkHexahedron.h"

vtkStructuredGrid::vtkStructuredGrid()
{
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;
  
  this->Blanking = 0;
  this->PointVisibility = NULL;
}

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

vtkStructuredGrid::~vtkStructuredGrid()
{
  this->Initialize();
  if (this->PointVisibility) 
    {
    this->PointVisibility->UnRegister((vtkObject *)this);
    }
  this->PointVisibility = NULL;
}

// Description:
// Copy the geometric and topological structure of an input structured grid.
void vtkStructuredGrid::CopyStructure(vtkDataSet *ds)
{
  vtkStructuredGrid *sg=(vtkStructuredGrid *)ds;
  vtkPointSet::CopyStructure(ds);

  for (int i=0; i<3; i++)
    {
    this->Dimensions[i] = sg->Dimensions[i];
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

void vtkStructuredGrid::Initialize()
{
  vtkPointSet::Initialize(); 
  if ( this->PointVisibility ) 
    this->PointVisibility->UnRegister(this);
  this->PointVisibility = NULL;
  this->Blanking = 0;
}

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
      return VTK_NULL_ELEMENT;
    }
}

vtkCell *vtkStructuredGrid::GetCell(int cellId)
{
  static vtkVertex vertex;
  static vtkLine line;
  static vtkQuad quad;
  static vtkHexahedron hexa;
  static vtkCell *cell;
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
      cell = &vertex;
      cell->PointIds.InsertId(0,0);
      break;

    case VTK_X_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case VTK_Y_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case VTK_Z_LINE:
      cell = &line;
      cell->PointIds.InsertId(0,cellId);
      cell->PointIds.InsertId(1,cellId+1);
      break;

    case VTK_XY_PLANE:
      cell = &quad;
      i = cellId % (this->Dimensions[0]-1);
      j = cellId / (this->Dimensions[0]-1);
      idx = i + j*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case VTK_YZ_PLANE:
      cell = &quad;
      j = cellId % (this->Dimensions[1]-1);
      k = cellId / (this->Dimensions[1]-1);
      idx = j + k*this->Dimensions[1];
      offset1 = 1;
      offset2 = this->Dimensions[1];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case VTK_XZ_PLANE:
      cell = &quad;
      i = cellId % (this->Dimensions[0]-1);
      k = cellId / (this->Dimensions[0]-1);
      idx = i + k*this->Dimensions[0];
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      break;

    case VTK_XYZ_GRID:
      cell = &hexa;
      d01 = this->Dimensions[0]*this->Dimensions[1];
      i = cellId % (this->Dimensions[0] - 1);
      j = (cellId / (this->Dimensions[0] - 1)) % (this->Dimensions[1] - 1);
      k = cellId / ((this->Dimensions[0] - 1) * (this->Dimensions[1] - 1));
      idx = i+ j*this->Dimensions[0] + k*d01;
      offset1 = 1;
      offset2 = this->Dimensions[0];

      cell->PointIds.InsertId(0,idx);
      cell->PointIds.InsertId(1,idx+offset1);
      cell->PointIds.InsertId(2,idx+offset1+offset2);
      cell->PointIds.InsertId(3,idx+offset2);
      idx += d01;
      cell->PointIds.InsertId(4,idx);
      cell->PointIds.InsertId(5,idx+offset1);
      cell->PointIds.InsertId(6,idx+offset1+offset2);
      cell->PointIds.InsertId(7,idx+offset2);
      break;
    }

  // Extract point coordinates and point ids. NOTE: the ordering of the vtkQuad
  // and vtkHexahedron cells are tricky.
  int NumberOfIds = cell->PointIds.GetNumberOfIds();
  for (i=0; i<NumberOfIds; i++)
    {
    idx = cell->PointIds.GetId(i);
    cell->Points.InsertPoint(i,this->Points->GetPoint(idx));
    }

  return cell;
}

// Description:
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

void vtkStructuredGrid::AllocatePointVisibility()
{
  if ( !this->PointVisibility )
    {
    this->PointVisibility = vtkBitScalars::New();
    this->PointVisibility->Allocate(this->GetNumberOfPoints(),1000);
    this->PointVisibility->Register((vtkObject *)this);
    for (int i=0; i<this->GetNumberOfPoints(); i++)
      {
      this->PointVisibility->InsertScalar(i,1);
      }
    this->PointVisibility->Delete();
    }
}

// Description:
// Turn off data blanking.
void vtkStructuredGrid::BlankingOff()
{
  if (this->Blanking)
    {
    this->Blanking = 0;
    this->Modified();
    }
}

// Description:
// Turn off a particular data point.
void vtkStructuredGrid::BlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->AllocatePointVisibility();
  this->PointVisibility->InsertScalar(ptId,0);
}

// Description:
// Turn on a particular data point.
void vtkStructuredGrid::UnBlankPoint(int ptId)
{
  if ( !this->PointVisibility ) this->AllocatePointVisibility();
  this->PointVisibility->InsertScalar(ptId,1);
}

// Description:
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;
  this->SetDimensions(dim);
}

// Description:
// Set dimensions of structured grid dataset.
void vtkStructuredGrid::SetDimensions(int dim[3])
{
  int returnStatus=vtkStructuredData::SetDimensions(dim,this->Dimensions);

  if ( returnStatus > 0 ) 
    {
    this->DataDescription = returnStatus;
    this->Modified();
    }

   else if ( returnStatus < 0 ) //improperly specified
    {
    vtkErrorMacro (<< "Bad Dimensions, retaining previous values");
    }
}

void vtkStructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", "
                                  << this->Dimensions[1] << ", "
                                  << this->Dimensions[2] << ")\n";

  os << indent << "Blanking: " << (this->Blanking ? "On\n" : "Off\n");
}

