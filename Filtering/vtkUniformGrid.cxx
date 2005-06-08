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
#include "vtkInformation.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStructuredVisibilityConstraint.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"

vtkCxxRevisionMacro(vtkUniformGrid, "1.10");
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
  this->PointVisibility = vtkStructuredVisibilityConstraint::New();
  this->CellVisibility = vtkStructuredVisibilityConstraint::New();

  this->EmptyCell = vtkEmptyCell::New();
}

//----------------------------------------------------------------------------
vtkUniformGrid::~vtkUniformGrid()
{
  this->PointVisibility->Delete();
  this->CellVisibility->Delete();
  this->EmptyCell->Delete();
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points 
// object.
void vtkUniformGrid::CopyStructure(vtkDataSet *ds)
{
  this->Initialize();

  this->Superclass::CopyStructure(ds);

  vtkUniformGrid *sPts=vtkUniformGrid::SafeDownCast(ds);
  if (!sPts)
    {
    return;
    }

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
  int extent[6];
  this->GetExtent(extent);

  vtkIdType cellId =  (loc[2]-extent[4]) * (dims[0]-1)*(dims[1]-1) +
    (loc[1]-extent[2]) * (dims[0]-1) + loc[0] - extent[0];

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
void vtkUniformGrid::ShallowCopy(vtkDataObject *dataObject)
{
  vtkUniformGrid *ugData = vtkUniformGrid::SafeDownCast(dataObject);

  if ( ugData )
    {
    this->PointVisibility->ShallowCopy(ugData->PointVisibility);
    this->CellVisibility->ShallowCopy(ugData->CellVisibility);
    }

  // Do superclass
  this->Superclass::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkUniformGrid::DeepCopy(vtkDataObject *dataObject)
{
  vtkUniformGrid *ugData = vtkUniformGrid::SafeDownCast(dataObject);

  if ( ugData != NULL )
    {
    this->PointVisibility->DeepCopy(ugData->PointVisibility);
    this->CellVisibility->DeepCopy(ugData->CellVisibility);
    }
  // Do superclass
  this->Superclass::DeepCopy(dataObject);
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
