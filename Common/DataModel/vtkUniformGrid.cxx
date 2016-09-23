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

#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkEmptyCell.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"

vtkStandardNewMacro(vtkUniformGrid);

unsigned char vtkUniformGrid::MASKED_CELL_VALUE =
  vtkDataSetAttributes::HIDDENCELL | vtkDataSetAttributes::REFINEDCELL;

//----------------------------------------------------------------------------
vtkUniformGrid::vtkUniformGrid()
{
  this->EmptyCell = NULL;
}

//----------------------------------------------------------------------------
vtkUniformGrid::~vtkUniformGrid()
{
  if (this->EmptyCell)
  {
    this->EmptyCell->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkUniformGrid::Initialize()
{
  this->Superclass::Initialize();
}

//-----------------------------------------------------------------------------
int vtkUniformGrid::Initialize(const vtkAMRBox *def, double* origin, double* spacing)
{
  if (def->Empty())
  {
    vtkWarningMacro("Can't construct a data set from an empty box.");
    return 0;
  }
  if (def->ComputeDimension()==2)
  {
    // NOTE: Define it 3D, with the third dim 0. eg. (X,X,0)(X,X,0)
    vtkWarningMacro("Can't construct a 3D data set from a 2D box.");
    return 0;
  }

  this->Initialize();
  int nPoints[3];
  def->GetNumberOfNodes(nPoints);

  this->SetDimensions(nPoints);
  this->SetSpacing(spacing);
  this->SetOrigin(origin);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkUniformGrid::Initialize(
        const vtkAMRBox *def,
        double* origin,
        double* spacing,
        int nGhostsI,
        int nGhostsJ,
        int nGhostsK)
{
  if (!this->Initialize(def,origin,spacing))
  {
    return 0;
  }

  // Generate ghost cell array, with no ghosts marked.
  int nCells[3];
  def->GetNumberOfCells(nCells);
  vtkUnsignedCharArray *ghosts=vtkUnsignedCharArray::New();
  this->GetCellData()->AddArray(ghosts);
  ghosts->Delete();
  ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
  ghosts->SetNumberOfComponents(1);
  ghosts->SetNumberOfTuples(nCells[0]*nCells[1]*nCells[2]);
  ghosts->FillComponent(0,0);
  // If there are ghost cells mark them.
  if (nGhostsI || nGhostsJ || nGhostsK)
  {
    unsigned char *pG=ghosts->GetPointer(0);
    const int* lo = def->GetLoCorner();
    const int* hi = def->GetHiCorner();
    // Identify & fill ghost regions
    if (nGhostsI)
    {
      vtkAMRBox left(lo[0],lo[1],lo[2],lo[0]+nGhostsI-1,hi[1],hi[2]);
      FillRegion(pG,*def,left,static_cast<unsigned char>(1));
      vtkAMRBox right(hi[0]-nGhostsI+1,lo[1],lo[2],hi[0],hi[1],hi[2]);
      FillRegion(pG,*def,right,static_cast<unsigned char>(1));
    }
    if (nGhostsJ)
    {
      vtkAMRBox front(lo[0],lo[1],lo[2],hi[0],lo[1]+nGhostsJ-1,hi[2]);
      FillRegion(pG,*def,front,static_cast<unsigned char>(1));
      vtkAMRBox back(lo[0],hi[1]-nGhostsJ+1,lo[2],hi[0],hi[1],hi[2]);
      FillRegion(pG,*def,back,static_cast<unsigned char>(1));
    }
    if (nGhostsK)
    {
      vtkAMRBox bottom(lo[0],lo[1],lo[2],hi[0],hi[1],lo[2]+nGhostsK-1);
      FillRegion(pG,*def,bottom,static_cast<unsigned char>(1));
      vtkAMRBox top(lo[0],lo[1],hi[2]-nGhostsK+1,hi[0],hi[1],hi[2]);
      FillRegion(pG,*def,top,static_cast<unsigned char>(1));
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkUniformGrid::Initialize(const vtkAMRBox *def, double* origin, double* spacing,const int nGhosts[3])
{
  return this->Initialize(def, origin, spacing, nGhosts[0],nGhosts[1],nGhosts[2]);
}

//-----------------------------------------------------------------------------
int vtkUniformGrid::Initialize(const vtkAMRBox *def, double* origin, double* spacing, int nGhosts)
{
  return this->Initialize(def, origin, spacing, nGhosts,nGhosts,nGhosts);
}

//----------------------------------------------------------------------------
int vtkUniformGrid::GetGridDescription()
{
  return( this->GetDataDescription() );
}

//----------------------------------------------------------------------------
vtkEmptyCell* vtkUniformGrid::GetEmptyCell()
{
  if (!this->EmptyCell)
  {
    this->EmptyCell = vtkEmptyCell::New();
  }
  return this->EmptyCell;
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points
// object.
void vtkUniformGrid::CopyStructure(vtkDataSet *ds)
{
  this->Initialize();

  this->Superclass::CopyStructure(ds);

  if(ds->HasAnyBlankPoints())
  {
    // there is blanking
    this->GetPointData()->AddArray(ds->GetPointGhostArray());
    this->PointGhostArray = NULL;
  }
  if(ds->HasAnyBlankCells())
  {
    // we assume there is blanking
    this->GetCellData()->AddArray(ds->GetCellGhostArray());
    this->CellGhostArray = NULL;
  }
}

//----------------------------------------------------------------------------
vtkCell *vtkUniformGrid::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int extent[6];
  this->GetExtent(extent);

  int dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  int d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return this->GetEmptyCell();
  }

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId) )
  {
    return this->GetEmptyCell();
  }

  switch (this->GetDataDescription())
  {
    case VTK_EMPTY:
      return this->GetEmptyCell();

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
      vtkErrorMacro(<<"Invalid DataDescription.");
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
void vtkUniformGrid::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType npts, idx;
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  double x[3];
  int extent[6];
  this->GetExtent(extent);

  int dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  int d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
  {
    vtkErrorMacro("Requesting a cell from an empty image.");
    cell->SetCellTypeToEmptyCell();
    return;
  }

  // see whether the cell is blanked
  if (!this->IsCellVisible(cellId))
  {
    cell->SetCellTypeToEmptyCell();
    return;
  }

  switch (this->GetDataDescription())
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
vtkIdType vtkUniformGrid::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                   vtkGenericCell *vtkNotUsed(gencell),
                                   vtkIdType vtkNotUsed(cellId),
                                   double vtkNotUsed(tol2),
                                   int& subId, double pcoords[3],
                                   double *weights)
{
  return
    this->FindCell( x, static_cast<vtkCell *>(NULL), 0, 0.0, subId, pcoords,
                    weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkUniformGrid::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                 vtkIdType vtkNotUsed(cellId),
                                 double vtkNotUsed(tol2),
                                 int& subId, double pcoords[3],
                                   double *weights)
{
  int loc[3];
  int *dims = this->GetDimensions();

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
  {
    return -1;
  }

  this->Voxel->InterpolationFunctions(pcoords,weights);

  //
  //  From this location get the cell id
  //
  subId = 0;
  int extent[6];
  this->GetExtent(extent);

  vtkIdType cellId =  (loc[2]-extent[4]) * (dims[0]-1)*(dims[1]-1) +
    (loc[1]-extent[2]) * (dims[0]-1) + loc[0] - extent[0];

  if ( (this->GetPointGhostArray() ||
        this->GetCellGhostArray())
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
  double xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  double *origin = this->GetOrigin();
  double *spacing = this->GetSpacing();
  int extent[6];
  this->GetExtent(extent);

  int dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType d01 = dims[0]*dims[1];

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
  {
    return NULL;
  }

  vtkIdType cellId = loc[2] * (dims[0]-1)*(dims[1]-1) +
    loc[1] * (dims[0]-1) + loc[0];

  if (!this->IsCellVisible(cellId))
  {
    return NULL;
  }

  //
  // Get the parametric coordinates and weights for interpolation
  //
  switch (this->GetDataDescription())
  {
    case VTK_EMPTY:
      return NULL;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Voxel;
      break;

    default:
      vtkErrorMacro(<<"Invalid DataDescription.");
      return NULL;
  }
  cell->InterpolateFunctions(pcoords,weights);

  npts = 0;
  for (k = loc[2]; k <= kMax; k++)
  {
    xOut[2] = origin[2] + k * spacing[2];
    for (j = loc[1]; j <= jMax; j++)
    {
      xOut[1] = origin[1] + j * spacing[1];
      // make idx relative to the extent not the whole extent
      idx = loc[0]-extent[0] + (j-extent[2])*dims[0]
        + (k-extent[4])*d01;
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
  if (!this->IsCellVisible(cellId) )
  {
    return VTK_EMPTY_CELL;
  }

  switch (this->GetDataDescription())
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
void vtkUniformGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkImageData* vtkUniformGrid::NewImageDataCopy()
{
  vtkImageData* copy = vtkImageData::New();

  copy->ShallowCopy(this);

  double origin[3];
  double spacing[3];
  this->GetOrigin(origin);
  this->GetSpacing(spacing);
  // First set the extent of the copy to empty so that
  // the next call computes the DataDescription for us
  copy->SetExtent(0, -1, 0, -1, 0, -1);
  copy->SetExtent(this->GetExtent());
  copy->SetOrigin(origin);
  copy->SetSpacing(spacing);

  return copy;
}

//----------------------------------------------------------------------------
// Override this method because of blanking
void vtkUniformGrid::ComputeScalarRange()
{
  if ( this->GetMTime() > this->ScalarRangeComputeTime )
  {
    vtkDataArray *ptScalars = this->PointData->GetScalars();
    vtkDataArray *cellScalars = this->CellData->GetScalars();
    double ptRange[2];
    double cellRange[2];
    double s;
    int id, num;

    ptRange[0] =  VTK_DOUBLE_MAX;
    ptRange[1] =  VTK_DOUBLE_MIN;
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

    this->ScalarRange[0] = (cellRange[0] >= VTK_DOUBLE_MAX ? 0.0 : cellRange[0]);
    this->ScalarRange[1] = (cellRange[1] <= VTK_DOUBLE_MIN ? 1.0 : cellRange[1]);
    this->ScalarRangeComputeTime.Modified();
  }
}

//----------------------------------------------------------------------------
// Turn off a particular data point.
void vtkUniformGrid::BlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if(!ghosts)
  {
    this->AllocatePointGhostArray();
    ghosts = this->GetPointGhostArray();
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) | vtkDataSetAttributes::HIDDENPOINT);
  assert(!this->IsPointVisible(ptId));
}

//----------------------------------------------------------------------------
void vtkUniformGrid::BlankPoint( const int i, const int j, const int k )
{
  int ijk[3];
  ijk[0] = i; ijk[1] = j; ijk[2] = k;
  int idx = vtkStructuredData::ComputePointId(this->Dimensions, ijk );
  this->BlankPoint( idx );
}

//----------------------------------------------------------------------------
// Turn on a particular data point.
void vtkUniformGrid::UnBlankPoint(vtkIdType ptId)
{
  vtkUnsignedCharArray* ghosts = this->GetPointGhostArray();
  if(!ghosts)
  {
    return;
  }
  ghosts->SetValue(ptId, ghosts->GetValue(ptId) &
                   ~vtkDataSetAttributes::HIDDENPOINT);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::UnBlankPoint( const int i, const int j, const int k )
{
  int ijk[3];
  ijk[0] = i; ijk[1] = j; ijk[2] = k;
  int idx = vtkStructuredData::ComputePointId(this->Dimensions, ijk );
  this->UnBlankPoint( idx );
}

//----------------------------------------------------------------------------
// Turn off a particular data cell.
void vtkUniformGrid::BlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghost = this->GetCellGhostArray();
  if(!ghost)
  {
    this->AllocateCellGhostArray();
    ghost = this->GetCellGhostArray();
  }
  ghost->SetValue(cellId, ghost->GetValue(cellId) |
                  vtkDataSetAttributes::HIDDENCELL);
  assert(!this->IsCellVisible(cellId));
}

//----------------------------------------------------------------------------
void vtkUniformGrid::BlankCell( const int i, const int j, const int k )
{
  int ijk[3];
  ijk[0] = i; ijk[1] = j; ijk[2] = k;
  int idx = vtkStructuredData::ComputeCellId(this->Dimensions, ijk );
  assert( "cell id in range:" &&
   ( (idx >= 0) && (idx < this->GetNumberOfCells() ) ) );
  this->BlankCell( idx );
}

//----------------------------------------------------------------------------
// Turn on a particular data cell.
void vtkUniformGrid::UnBlankCell(vtkIdType cellId)
{
  vtkUnsignedCharArray* ghosts = this->GetCellGhostArray();
  if(!ghosts)
  {
    return;
  }
  ghosts->SetValue(cellId, ghosts->GetValue(cellId) &
                   ~vtkDataSetAttributes::HIDDENCELL);
  assert(this->IsCellVisible(cellId));
}

//----------------------------------------------------------------------------
void vtkUniformGrid::UnBlankCell( const int i, const int j, const int k )
{
  int ijk[3];
  ijk[0] = i; ijk[1] = j; ijk[2] = k;
  int idx = vtkStructuredData::ComputeCellId(this->Dimensions, ijk );
  assert( "cell id in range:" &&
    ( (idx >= 0) && (idx < this->GetNumberOfCells() ) ) );
  this->UnBlankCell( idx );
}

//----------------------------------------------------------------------------
unsigned char vtkUniformGrid::IsPointVisible(vtkIdType pointId)
{
  if (this->GetPointGhostArray() &&
      (this->GetPointGhostArray()->GetValue(pointId) &
       vtkDataSetAttributes::HIDDENPOINT))
  {
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
// Return non-zero if the specified cell is visible (i.e., not blanked)
unsigned char vtkUniformGrid::IsCellVisible(vtkIdType cellId)
{

  if (this->GetCellGhostArray() &&
      (this->GetCellGhostArray()->GetValue(cellId) & MASKED_CELL_VALUE))
  {
    return 0;
  }
  if (! this->GetPointGhostArray())
  {
    return (this->GetDataDescription() == VTK_EMPTY) ? 0 : 1;
  }

  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch (this->GetDataDescription())
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
void vtkUniformGrid::GetCellDims( int cellDims[3] )
{
  int nodeDims[3];
  this->GetDimensions( nodeDims );
  for( int i=0; i < 3; ++i )
  {
    cellDims[i] = ( (nodeDims[i]-1) < 1)? 1 : nodeDims[i]-1;
  }
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformation* info)
{
  return info ? vtkUniformGrid::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkUniformGrid::GetData(vtkInformationVector* v, int i)
{
  return vtkUniformGrid::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
bool vtkUniformGrid::HasAnyBlankPoints()
{
  return IsAnyBitSet(
    this->GetPointGhostArray(), vtkDataSetAttributes::HIDDENPOINT);
}

//----------------------------------------------------------------------------
bool vtkUniformGrid::HasAnyBlankCells()
{
  int cellBlanking = IsAnyBitSet(this->GetCellGhostArray(),
                                 vtkDataSetAttributes::HIDDENCELL);
  return cellBlanking || this->HasAnyBlankPoints();
}
