/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSet.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataSetCellIterator.h"
#include "vtkExtentTranslator.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkPointData.h"

#include <math.h>


//----------------------------------------------------------------------------
// Constructor with default bounds (0,1, 0,1, 0,1).
vtkDataSet::vtkDataSet ()
{
  vtkMath::UninitializeBounds(this->Bounds);

  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//----------------------------------------------------------------------------
vtkDataSet::~vtkDataSet ()
{
  this->PointData->Delete();
  this->CellData->Delete();
}

//----------------------------------------------------------------------------
void vtkDataSet::Initialize()
{
  // We don't modify ourselves because the "ReleaseData" methods depend upon
  // no modification when initialized.
  vtkDataObject::Initialize();

  this->CellData->Initialize();
  this->PointData->Initialize();
}

//----------------------------------------------------------------------------
void vtkDataSet::CopyAttributes(vtkDataSet *ds)
{
  this->GetPointData()->PassData(ds->GetPointData());
  this->GetCellData()->PassData(ds->GetCellData());
  this->GetFieldData()->PassData(ds->GetFieldData());
}

//----------------------------------------------------------------------------
vtkCellIterator *vtkDataSet::NewCellIterator()
{
  vtkDataSetCellIterator *iter = vtkDataSetCellIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//----------------------------------------------------------------------------
// Compute the data bounding box from data points.
void vtkDataSet::ComputeBounds()
{
  int j;
  vtkIdType i;
  double *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    if (this->GetNumberOfPoints())
      {
      x = this->GetPoint(0);
      this->Bounds[0] = this->Bounds[1] = x[0];
      this->Bounds[2] = this->Bounds[3] = x[1];
      this->Bounds[4] = this->Bounds[5] = x[2];
      for (i=1; i<this->GetNumberOfPoints(); i++)
        {
        x = this->GetPoint(i);
        for (j=0; j<3; j++)
          {
          if ( x[j] < this->Bounds[2*j] )
            {
            this->Bounds[2*j] = x[j];
            }
          if ( x[j] > this->Bounds[2*j+1] )
            {
            this->Bounds[2*j+1] = x[j];
            }
          }
        }
      }
    else
      {
      vtkMath::UninitializeBounds(this->Bounds);
      }
    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// Compute the range of the scalars and cache it into ScalarRange
// only if the cache became invalid (ScalarRangeComputeTime).
void vtkDataSet::ComputeScalarRange()
{
  if ( this->GetMTime() > this->ScalarRangeComputeTime )
    {
    vtkDataArray *ptScalars, *cellScalars;
    ptScalars = this->PointData->GetScalars();
    cellScalars = this->CellData->GetScalars();

    if ( ptScalars && cellScalars)
      {
      double r1[2], r2[2];
      ptScalars->GetRange(r1,0);
      cellScalars->GetRange(r2,0);
      this->ScalarRange[0] = (r1[0] < r2[0] ? r1[0] : r2[0]);
      this->ScalarRange[1] = (r1[1] > r2[1] ? r1[1] : r2[1]);
      }
    else if ( ptScalars )
      {
      ptScalars->GetRange(this->ScalarRange,0);
      }
    else if ( cellScalars )
      {
      cellScalars->GetRange(this->ScalarRange,0);
      }
    else
      {
      this->ScalarRange[0] = 0.0;
      this->ScalarRange[1] = 1.0;
      }
    this->ScalarRangeComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkDataSet::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0]=this->ScalarRange[0];
  range[1]=this->ScalarRange[1];
}

//----------------------------------------------------------------------------
double *vtkDataSet::GetScalarRange()
{
  this->ComputeScalarRange();
  return this->ScalarRange;
}

//----------------------------------------------------------------------------
// Return a pointer to the geometry bounding box in the form
// (xmin,xmax, ymin,ymax, zmin,zmax).
double *vtkDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkDataSet::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

//----------------------------------------------------------------------------
// Get the center of the bounding box.
double *vtkDataSet::GetCenter()
{
  this->ComputeBounds();
  for (int i=0; i<3; i++)
    {
    this->Center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
    }
  return this->Center;
}

//----------------------------------------------------------------------------
void vtkDataSet::GetCenter(double center[3])
{
  this->ComputeBounds();
  for (int i=0; i<3; i++)
    {
    center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
    }
}

//----------------------------------------------------------------------------
// Return the length of the diagonal of the bounding box.
double vtkDataSet::GetLength()
{
  if (this->GetNumberOfPoints() == 0)
    {
    return 0;
    }

  double diff, l=0.0;
  int i;

  this->ComputeBounds();
  for (i=0; i<3; i++)
    {
    diff = static_cast<double>(this->Bounds[2*i+1]) -
      static_cast<double>(this->Bounds[2*i]);
    l += diff * diff;
    }
  diff = sqrt(l);
  return diff;
}

//----------------------------------------------------------------------------
unsigned long int vtkDataSet::GetMTime()
{
  unsigned long mtime, result;

  result = vtkDataObject::GetMTime();

  mtime = this->PointData->GetMTime();
  result = ( mtime > result ? mtime : result );

  mtime = this->CellData->GetMTime();
  return ( mtime > result ? mtime : result );
}

//----------------------------------------------------------------------------
vtkCell *vtkDataSet::FindAndGetCell (double x[3], vtkCell *cell,
                                     vtkIdType cellId, double tol2, int& subId,
                                     double pcoords[3], double *weights)
{
  vtkIdType newCell = this->FindCell(x,cell,cellId,tol2,subId,pcoords,weights);
  if (newCell >= 0 )
    {
    cell = this->GetCell (newCell);
    }
  else
    {
    return NULL;
    }
  return cell;
}

//----------------------------------------------------------------------------
void vtkDataSet::GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                  vtkIdList *cellIds)
{
  vtkIdType i, numPts;
  vtkIdList *otherCells = vtkIdList::New();
  otherCells->Allocate(VTK_CELL_SIZE);

  // load list with candidate cells, remove current cell
  this->GetPointCells(ptIds->GetId(0), cellIds);
  cellIds->DeleteId(cellId);

  // now perform multiple intersections on list
  if ( cellIds->GetNumberOfIds() > 0 )
    {
    for ( numPts=ptIds->GetNumberOfIds(), i=1; i < numPts; i++)
      {
      this->GetPointCells(ptIds->GetId(i), otherCells);
      cellIds->IntersectWith(*otherCells);
      }
    }

  otherCells->Delete();
}

//----------------------------------------------------------------------------
void vtkDataSet::GetCellTypes(vtkCellTypes *types)
{
  vtkIdType cellId, numCells=this->GetNumberOfCells();
  unsigned char type;

  types->Reset();
  for (cellId=0; cellId < numCells; cellId++)
    {
    type = this->GetCellType(cellId);
    if ( ! types->IsType(type) )
      {
      types->InsertNextType(type);
      }
    }
}


//----------------------------------------------------------------------------
// Default implementation. This is very slow way to compute this information.
// Subclasses should override this method for efficiency.
void vtkDataSet::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  vtkGenericCell *cell = vtkGenericCell::New();

  this->GetCell(cellId, cell);
  cell->GetBounds(bounds);
  cell->Delete();
}

//----------------------------------------------------------------------------
void vtkDataSet::Squeeze()
{
  this->CellData->Squeeze();
  this->PointData->Squeeze();
}

//----------------------------------------------------------------------------
unsigned long vtkDataSet::GetActualMemorySize()
{
  unsigned long size = this->vtkDataObject::GetActualMemorySize();
  size += this->PointData->GetActualMemorySize();
  size += this->CellData->GetActualMemorySize();
  return size;
}

//----------------------------------------------------------------------------
void vtkDataSet::ShallowCopy(vtkDataObject *dataObject)
{
  vtkDataSet *dataSet = vtkDataSet::SafeDownCast(dataObject);

  if ( dataSet != NULL )
    {
    this->InternalDataSetCopy(dataSet);
    this->CellData->ShallowCopy(dataSet->GetCellData());
    this->PointData->ShallowCopy(dataSet->GetPointData());
    }
  // Do superclass
  this->vtkDataObject::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkDataSet::DeepCopy(vtkDataObject *dataObject)
{
  vtkDataSet *dataSet = vtkDataSet::SafeDownCast(dataObject);

  if ( dataSet != NULL )
    {
    this->InternalDataSetCopy(dataSet);
    this->CellData->DeepCopy(dataSet->GetCellData());
    this->PointData->DeepCopy(dataSet->GetPointData());
    }

  // Do superclass
  this->vtkDataObject::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkDataSet::InternalDataSetCopy(vtkDataSet *src)
{
  int idx;

  this->ScalarRangeComputeTime = src->ScalarRangeComputeTime;
  this->ScalarRange[0] = src->ScalarRange[0];
  this->ScalarRange[1] = src->ScalarRange[1];

  this->ComputeTime = src->ComputeTime;
  for (idx = 0; idx < 3; ++idx)
    {
    this->Bounds[2*idx] = src->Bounds[2*idx];
    this->Bounds[2*idx+1] = src->Bounds[2*idx+1];
    }
}


//----------------------------------------------------------------------------
int vtkDataSet::CheckAttributes()
{
  int numPts, numCells;
  int numArrays, idx;
  vtkAbstractArray *array;
  int numTuples;
  const char* name;

  numArrays = this->GetPointData()->GetNumberOfArrays();
  if (numArrays > 0)
    {
    // This call can be expensive.
    numPts = this->GetNumberOfPoints();
    for (idx = 0; idx < numArrays; ++idx)
      {
      array = this->GetPointData()->GetAbstractArray(idx);
      numTuples = array->GetNumberOfTuples();
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      if (numTuples < numPts)
        {
        vtkErrorMacro("Point array " << name << " with "
                      << array->GetNumberOfComponents()
                      << " components, only has " << numTuples << " tuples but there are "
                      << numPts << " points");
        return 1;
        }
      if (numTuples > numPts)
        {
        vtkWarningMacro("Point array " << name << " with "
                        << array->GetNumberOfComponents()
                        << " components, has " << numTuples << " tuples but there are only "
                        << numPts << " points");
        }
      }
    }

  numArrays = this->GetCellData()->GetNumberOfArrays();
  if (numArrays > 0)
    {
    // This call can be expensive.
    numCells = this->GetNumberOfCells();

    for (idx = 0; idx < numArrays; ++idx)
      {
      array = this->GetCellData()->GetAbstractArray(idx);
      numTuples = array->GetNumberOfTuples();
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      if (numTuples < numCells)
        {
        vtkErrorMacro("Cell array " << name << " with "
                      << array->GetNumberOfComponents()
                      << " components, has only " << numTuples << " tuples but there are "
                      << numCells << " cells");
        return 1;
        }
      if (numTuples > numCells)
        {
        vtkWarningMacro("Cell array " << name << " with "
                        << array->GetNumberOfComponents()
                        << " components, has " << numTuples << " tuples but there are only "
                        << numCells << " cells");
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkDataSet::GenerateGhostLevelArray(int update_piece,
                                         int update_num_pieces,
                                         int vtkNotUsed(update_ghost_level),
                                         int* whole_extent,
                                         vtkExtentTranslator* translator)
{
  // Make sure this is a structured data set.
  if(this->GetExtentType() != VTK_3D_EXTENT)
    {
    return;
    }

  // Avoid generating these if the producer has generated them.
  if(!this->PointData->GetArray("vtkGhostLevels"))
    { // Create ghost levels for cells and points.
    vtkUnsignedCharArray *levels;
    int zeroExt[6], extent[6];
    int i, j, k, di, dj, dk, dist;

    this->Information->Get(vtkDataObject::DATA_EXTENT(), extent);
    // Get the extent with ghost level 0.
    translator->SetWholeExtent(whole_extent);
    translator->SetPiece(update_piece);
    translator->SetNumberOfPieces(update_num_pieces);
    translator->SetGhostLevel(0);
    translator->PieceToExtent();
    translator->GetExtent(zeroExt);

    // ---- POINTS ----
    // Allocate the appropriate number levels (number of points).
    levels = vtkUnsignedCharArray::New();
    levels->Allocate((extent[1]-extent[0] + 1) *
                     (extent[3]-extent[2] + 1) *
                     (extent[5]-extent[4] + 1));

    // Loop through the points in this image.
    for (k = extent[4]; k <= extent[5]; ++k)
      {
      dk = 0;
      if (k < zeroExt[4])
        {
        dk = zeroExt[4] - k;
        }
      if (k >= zeroExt[5] && k < whole_extent[5])
        { // Special case for last tile.
        dk = k - zeroExt[5] + 1;
        }
      for (j = extent[2]; j <= extent[3]; ++j)
        {
        dj = 0;
        if (j < zeroExt[2])
          {
          dj = zeroExt[2] - j;
          }
        if (j >= zeroExt[3] && j < whole_extent[3])
          { // Special case for last tile.
          dj = j - zeroExt[3] + 1;
          }
        for (i = extent[0]; i <= extent[1]; ++i)
          {
          di = 0;
          if (i < zeroExt[0])
            {
            di = zeroExt[0] - i;
            }
          if (i >= zeroExt[1] && i < whole_extent[1])
            { // Special case for last tile.
            di = i - zeroExt[1] + 1;
            }
          // Compute Manhatten distance.
          dist = di;
          if (dj > dist)
            {
            dist = dj;
            }
          if (dk > dist)
            {
            dist = dk;
            }

          //cerr << "   " << i << ", " << j << ", " << k << endl;
          //cerr << "   " << di << ", " << dj << ", " << dk << endl;
          //cerr << dist << endl;

          levels->InsertNextValue(static_cast<unsigned char>(dist));
          }
        }
      }
    levels->SetName("vtkGhostLevels");
    this->PointData->AddArray(levels);
    levels->Delete();

    // ---- CELLS ----
    // Allocate the appropriate number levels (number of cells).
    levels = vtkUnsignedCharArray::New();
    levels->Allocate((extent[1]-extent[0]) *
                     (extent[3]-extent[2]) *
                     (extent[5]-extent[4]));

    // Loop through the cells in this image.
    // Cells may be 2d or 1d ... Treat all as 3D
    if (extent[0] == extent[1])
      {
      ++extent[1];
      ++zeroExt[1];
      }
    if (extent[2] == extent[3])
      {
      ++extent[3];
      ++zeroExt[3];
      }
    if (extent[4] == extent[5])
      {
      ++extent[5];
      ++zeroExt[5];
      }

    // Loop
    for (k = extent[4]; k < extent[5]; ++k)
      { // Determine the Manhatten distances to zero extent.
      dk = 0;
      if (k < zeroExt[4])
        {
        dk = zeroExt[4] - k;
        }
      if (k >= zeroExt[5])
        {
        dk = k - zeroExt[5] + 1;
        }
      for (j = extent[2]; j < extent[3]; ++j)
        {
        dj = 0;
        if (j < zeroExt[2])
          {
          dj = zeroExt[2] - j;
          }
        if (j >= zeroExt[3])
          {
          dj = j - zeroExt[3] + 1;
          }
        for (i = extent[0]; i < extent[1]; ++i)
          {
          di = 0;
          if (i < zeroExt[0])
            {
            di = zeroExt[0] - i;
            }
          if (i >= zeroExt[1])
            {
            di = i - zeroExt[1] + 1;
            }
          // Compute Manhatten distance.
          dist = di;
          if (dj > dist)
            {
            dist = dj;
            }
          if (dk > dist)
            {
            dist = dk;
            }

          levels->InsertNextValue(static_cast<unsigned char>(dist));
          }
        }
      }
    levels->SetName("vtkGhostLevels");
    this->CellData->AddArray(levels);
    levels->Delete();
    }

}

//----------------------------------------------------------------------------
vtkDataSet* vtkDataSet::GetData(vtkInformation* info)
{
  return info? vtkDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
vtkFieldData* vtkDataSet::GetAttributesAsFieldData(int type)
{
  switch(type)
    {
    case POINT:
      return this->GetPointData();
      break;
    case CELL:
      return this->GetCellData();
      break;
    }
  return this->Superclass::GetAttributesAsFieldData(type);
}

//----------------------------------------------------------------------------
vtkIdType vtkDataSet::GetNumberOfElements(int type)
{
  switch (type)
    {
    case POINT:
      return this->GetNumberOfPoints();
      break;
    case CELL:
      return this->GetNumberOfCells();
      break;
    }
  return this->Superclass::GetNumberOfElements(type);;
}

//----------------------------------------------------------------------------
void vtkDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  double *bounds;

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Number Of Cells: " << this->GetNumberOfCells() << "\n";

  os << indent << "Cell Data:\n";
  this->CellData->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Point Data:\n";
  this->PointData->PrintSelf(os,indent.GetNextIndent());

  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" <<bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" <<bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" <<bounds[4] << ", " << bounds[5] << ")\n";
  os << indent << "Compute Time: " <<this->ComputeTime.GetMTime() << "\n";
}

