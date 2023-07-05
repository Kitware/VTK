// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0
#include "vtkDataSet.h"

#include "vtkBezierCurve.h"
#include "vtkBezierHexahedron.h"
#include "vtkBezierQuadrilateral.h"
#include "vtkBezierTetra.h"
#include "vtkBezierTriangle.h"
#include "vtkBezierWedge.h"
#include "vtkCallbackCommand.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataSetCellIterator.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangeQuadrilateral.h"
#include "vtkLagrangeWedge.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"

#include <cmath>
#include <set>

//------------------------------------------------------------------------------
// Constructor with default bounds (0,1, 0,1, 0,1).
VTK_ABI_NAMESPACE_BEGIN
vtkDataSet::vtkDataSet()
{
  vtkMath::UninitializeBounds(this->Bounds);
  // Observer for updating the cell/point ghost arrays pointers
  this->DataObserver = vtkCallbackCommand::New();
  this->DataObserver->SetCallback(&vtkDataSet::OnDataModified);
  this->DataObserver->SetClientData(this);

  this->PointData = vtkPointData::New();
  // when point data is modified, we update the point data ghost array cache
  this->PointData->AddObserver(vtkCommand::ModifiedEvent, this->DataObserver);

  this->CellData = vtkCellData::New();
  // when cell data is modified, we update the cell data ghost array cache
  this->CellData->AddObserver(vtkCommand::ModifiedEvent, this->DataObserver);

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//------------------------------------------------------------------------------
vtkDataSet::~vtkDataSet()
{
  this->PointData->RemoveObserver(this->DataObserver);
  this->PointData->Delete();

  this->CellData->RemoveObserver(this->DataObserver);
  this->CellData->Delete();

  this->DataObserver->Delete();
}

//------------------------------------------------------------------------------
void vtkDataSet::Initialize()
{
  // We don't modify ourselves because the "ReleaseData" methods depend upon
  // no modification when initialized.
  vtkDataObject::Initialize();

  this->CellData->Initialize();
  this->PointData->Initialize();
}

//------------------------------------------------------------------------------
void vtkDataSet::CopyAttributes(vtkDataSet* ds)
{
  this->GetPointData()->PassData(ds->GetPointData());
  this->GetCellData()->PassData(ds->GetCellData());
  this->GetFieldData()->PassData(ds->GetFieldData());
}

//------------------------------------------------------------------------------
vtkCellIterator* vtkDataSet::NewCellIterator()
{
  vtkDataSetCellIterator* iter = vtkDataSetCellIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//------------------------------------------------------------------------------
struct ComputeBoundsFunctor
{
  vtkDataSet* DataSet;
  vtkSMPThreadLocal<std::array<double, 6>> TLBounds;
  std::array<double, 6> Bounds{};

  ComputeBoundsFunctor(vtkDataSet* dataset)
    : DataSet(dataset)
  {
  }

  void Initialize()
  {
    auto& bounds = this->TLBounds.Local();
    bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] = VTK_DOUBLE_MIN;
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    double x[3];
    uint8_t j;
    auto& bounds = this->TLBounds.Local();
    for (vtkIdType pointId = begin; pointId < end; ++pointId)
    {
      this->DataSet->GetPoint(pointId, x);
      for (j = 0; j < 3; j++)
      {
        if (x[j] < bounds[2 * j])
        {
          bounds[2 * j] = x[j];
        }
        if (x[j] > bounds[2 * j + 1])
        {
          bounds[2 * j + 1] = x[j];
        }
      }
    }
  }

  void Reduce()
  {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
    for (const auto& bounds : this->TLBounds)
    {
      for (uint8_t j = 0; j < 3; j++)
      {
        if (bounds[2 * j] < this->Bounds[2 * j])
        {
          this->Bounds[2 * j] = bounds[2 * j];
        }
        if (bounds[2 * j + 1] > this->Bounds[2 * j + 1])
        {
          this->Bounds[2 * j + 1] = bounds[2 * j + 1];
        }
      }
    }
  }
};

//------------------------------------------------------------------------------
// Compute the data bounding box from data points.
void vtkDataSet::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
  {
    if (this->GetNumberOfPoints())
    {
      ComputeBoundsFunctor functor(this);
      vtkSMPTools::For(0, this->GetNumberOfPoints(), functor);
      std::copy(functor.Bounds.begin(), functor.Bounds.end(), this->Bounds);
    }
    else
    {
      vtkMath::UninitializeBounds(this->Bounds);
    }
    this->ComputeTime.Modified();
  }
}

//------------------------------------------------------------------------------
// Description:
// Compute the range of the scalars and cache it into ScalarRange
// only if the cache became invalid (ScalarRangeComputeTime).
void vtkDataSet::ComputeScalarRange()
{
  if (this->GetMTime() > this->ScalarRangeComputeTime)
  {
    vtkDataArray *ptScalars, *cellScalars;
    ptScalars = this->PointData->GetScalars();
    cellScalars = this->CellData->GetScalars();

    vtkUnsignedCharArray* ptGhosts = this->PointData->GetGhostArray();
    const unsigned char* ptGhostsPtr = ptGhosts ? ptGhosts->GetPointer(0) : nullptr;
    unsigned char ptGhostsToSkip = this->PointData->GetGhostsToSkip();

    vtkUnsignedCharArray* cellGhosts = this->CellData->GetGhostArray();
    const unsigned char* cellGhostsPtr = cellGhosts ? cellGhosts->GetPointer(0) : nullptr;
    unsigned char cellGhostsToSkip = this->CellData->GetGhostsToSkip();

    double r1[2], r2[2];
    if (ptScalars && cellScalars)
    {
      ptScalars->GetRange(r1, 0, ptGhostsPtr, ptGhostsToSkip);
      cellScalars->GetRange(r2, 0, cellGhostsPtr, cellGhostsToSkip);
      this->ScalarRange[0] = (r1[0] < r2[0] ? r1[0] : r2[0]);
      this->ScalarRange[1] = (r1[1] > r2[1] ? r1[1] : r2[1]);
    }
    else if (ptScalars)
    {
      ptScalars->GetRange(this->ScalarRange, 0, ptGhostsPtr, ptGhostsToSkip);
    }
    else if (cellScalars)
    {
      cellScalars->GetRange(this->ScalarRange, 0, cellGhostsPtr, cellGhostsToSkip);
    }
    else
    {
      this->ScalarRange[0] = 0.0;
      this->ScalarRange[1] = 1.0;
    }
    this->ScalarRangeComputeTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkDataSet::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0] = this->ScalarRange[0];
  range[1] = this->ScalarRange[1];
}

//------------------------------------------------------------------------------
double* vtkDataSet::GetScalarRange()
{
  this->ComputeScalarRange();
  return this->ScalarRange;
}

//------------------------------------------------------------------------------
// Return a pointer to the geometry bounding box in the form
// (xmin,xmax, ymin,ymax, zmin,zmax).
double* vtkDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//------------------------------------------------------------------------------
void vtkDataSet::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->Bounds[i];
  }
}

//------------------------------------------------------------------------------
// Get the center of the bounding box.
double* vtkDataSet::GetCenter()
{
  this->ComputeBounds();
  for (int i = 0; i < 3; i++)
  {
    this->Center[i] = (this->Bounds[2 * i + 1] + this->Bounds[2 * i]) / 2.0;
  }
  return this->Center;
}

//------------------------------------------------------------------------------
void vtkDataSet::GetCenter(double center[3])
{
  this->ComputeBounds();
  for (int i = 0; i < 3; i++)
  {
    center[i] = (this->Bounds[2 * i + 1] + this->Bounds[2 * i]) / 2.0;
  }
}

//------------------------------------------------------------------------------
// Return the length of the diagonal of the bounding box.
double vtkDataSet::GetLength()
{
  return std::sqrt(this->GetLength2());
}

//------------------------------------------------------------------------------
// Return the squared length of the diagonal of the bounding box.
double vtkDataSet::GetLength2()
{
  if (this->GetNumberOfPoints() == 0)
  {
    return 0;
  }

  double diff, l = 0.0;
  int i;

  this->ComputeBounds();
  for (i = 0; i < 3; i++)
  {
    diff = static_cast<double>(this->Bounds[2 * i + 1]) - static_cast<double>(this->Bounds[2 * i]);
    l += diff * diff;
  }
  return l;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkDataSet::GetMTime()
{
  vtkMTimeType mtime, result;

  result = vtkDataObject::GetMTime();

  mtime = this->PointData->GetMTime();
  result = (mtime > result ? mtime : result);

  mtime = this->CellData->GetMTime();
  return (mtime > result ? mtime : result);
}

//------------------------------------------------------------------------------
vtkCell* vtkDataSet::FindAndGetCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2,
  int& subId, double pcoords[3], double* weights)
{
  vtkIdType newCell = this->FindCell(x, cell, cellId, tol2, subId, pcoords, weights);
  if (newCell >= 0)
  {
    cell = this->GetCell(newCell);
  }
  else
  {
    return nullptr;
  }
  return cell;
}

//------------------------------------------------------------------------------
void vtkDataSet::GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds)
{
  vtkNew<vtkIdList> otherCells;
  otherCells->Allocate(VTK_CELL_SIZE);

  // load list with candidate cells, remove current cell
  this->GetPointCells(ptIds->GetId(0), cellIds);
  cellIds->DeleteId(cellId);

  // now perform multiple intersections on list
  if (cellIds->GetNumberOfIds() > 0)
  {
    for (vtkIdType numPts = ptIds->GetNumberOfIds(), i = 1; i < numPts; i++)
    {
      this->GetPointCells(ptIds->GetId(i), otherCells);
      cellIds->IntersectWith(otherCells);
    }
  }
}

//------------------------------------------------------------------------------
int vtkDataSet::GetCellNumberOfFaces(
  vtkIdType cellId, unsigned char& cellType, vtkGenericCell* cell)
{
  cellType = static_cast<unsigned char>(this->GetCellType(cellId));
  switch (cellType)
  {
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
    case VTK_LINE:
    case VTK_POLY_LINE:
    case VTK_TRIANGLE:
    case VTK_TRIANGLE_STRIP:
    case VTK_POLYGON:
    case VTK_PIXEL:
    case VTK_QUAD:
    case VTK_QUADRATIC_EDGE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_QUADRATIC_QUAD:
    case VTK_QUADRATIC_POLYGON:
    case VTK_BIQUADRATIC_QUAD:
    case VTK_QUADRATIC_LINEAR_QUAD:
    case VTK_BIQUADRATIC_TRIANGLE:
    case VTK_CUBIC_LINE:
    case VTK_CONVEX_POINT_SET:
    case VTK_PARAMETRIC_CURVE:
    case VTK_PARAMETRIC_SURFACE:
    case VTK_PARAMETRIC_TRI_SURFACE:
    case VTK_PARAMETRIC_QUAD_SURFACE:
    case VTK_HIGHER_ORDER_EDGE:
    case VTK_HIGHER_ORDER_TRIANGLE:
    case VTK_HIGHER_ORDER_QUAD:
    case VTK_HIGHER_ORDER_POLYGON:
    case VTK_LAGRANGE_CURVE:
    case VTK_LAGRANGE_TRIANGLE:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_CURVE:
    case VTK_BEZIER_TRIANGLE:
    case VTK_BEZIER_QUADRILATERAL:
      return 0;

    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
    case VTK_PARAMETRIC_TETRA_REGION:
    case VTK_HIGHER_ORDER_TETRAHEDRON:
    case VTK_LAGRANGE_TETRAHEDRON:
    case VTK_BEZIER_TETRAHEDRON:
      return 4;

    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
    case VTK_TRIQUADRATIC_PYRAMID:
    case VTK_HIGHER_ORDER_PYRAMID:
    case VTK_WEDGE:
    case VTK_QUADRATIC_WEDGE:
    case VTK_QUADRATIC_LINEAR_WEDGE:
    case VTK_BIQUADRATIC_QUADRATIC_WEDGE:
    case VTK_HIGHER_ORDER_WEDGE:
    case VTK_LAGRANGE_WEDGE:
    case VTK_BEZIER_WEDGE:
      return 5;

    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_QUADRATIC_HEXAHEDRON:
    case VTK_TRIQUADRATIC_HEXAHEDRON:
    case VTK_HIGHER_ORDER_HEXAHEDRON:
    case VTK_PARAMETRIC_HEX_REGION:
    case VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
    case VTK_LAGRANGE_HEXAHEDRON:
    case VTK_BEZIER_HEXAHEDRON:
      return 6;

    case VTK_PENTAGONAL_PRISM:
      return 7;

    case VTK_HEXAGONAL_PRISM:
      return 8;

    case VTK_POLYHEDRON:
    default:
      this->GetCell(cellId, cell);
      return cell->GetNumberOfFaces();
  }
}

namespace
{
class DistinctCellTypesWorker
{
public:
  DistinctCellTypesWorker(vtkDataSet* ds)
    : DS(ds)
  {
    if (ds && ds->GetNumberOfCells() > 0)
    {
      // initialize internal data structures
      vtkNew<vtkGenericCell> cell;
      ds->GetCell(0, cell);
    }
  }

  vtkDataSet* DS;
  std::set<unsigned char> DistinctCellTypes;

  // Thread-local storage
  vtkSMPThreadLocal<std::set<unsigned char>> LocalDistinctCellTypes;

  void Initialize() {}

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (!this->DS)
    {
      return;
    }

    for (vtkIdType idx = begin; idx < end; ++idx)
    {
      unsigned char cellType = static_cast<unsigned char>(this->DS->GetCellType(idx));
      this->LocalDistinctCellTypes.Local().insert(cellType);
    }
  }

  void Reduce()
  {
    this->DistinctCellTypes.clear();
    for (const auto& distinctCellTypes : this->LocalDistinctCellTypes)
    {
      this->DistinctCellTypes.insert(distinctCellTypes.begin(), distinctCellTypes.end());
    }
  }
};
}

//------------------------------------------------------------------------------
void vtkDataSet::GetCellTypes(vtkCellTypes* types)
{
  DistinctCellTypesWorker worker(this);
  vtkSMPTools::For(0, this->GetNumberOfCells(), worker);
  if (types)
  {
    types->Reset();
    types->Allocate(static_cast<vtkIdType>(worker.DistinctCellTypes.size()));
    for (const auto& distinctCellType : worker.DistinctCellTypes)
    {
      types->InsertNextType(distinctCellType);
    }
  }
}

//------------------------------------------------------------------------------
void vtkDataSet::GetCellPoints(
  vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
{
  this->GetCellPoints(cellId, ptIds);
  npts = ptIds->GetNumberOfIds();
  pts = ptIds->GetPointer(0);
}

//------------------------------------------------------------------------------
void vtkDataSet::SetCellOrderAndRationalWeights(vtkIdType cellId, vtkGenericCell* cell)
{
  switch (cell->GetCellType())
  {
    // Set the degree for Lagrange elements
    case VTK_LAGRANGE_QUADRILATERAL:
    {
      vtkHigherOrderQuadrilateral* cellBezier =
        dynamic_cast<vtkHigherOrderQuadrilateral*>(cell->GetRepresentativeCell());
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1]);
      }
      else
      {
        vtkIdType numPts = cell->PointIds->GetNumberOfIds();
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }
      break;
    }
    case VTK_LAGRANGE_WEDGE:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkHigherOrderWedge* cellBezier =
        dynamic_cast<vtkHigherOrderWedge*>(cell->GetRepresentativeCell());
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1], degs[2], numPts);
      }
      else
      {
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }
      break;
    }
    case VTK_LAGRANGE_HEXAHEDRON:
    {
      vtkHigherOrderHexahedron* cellBezier =
        dynamic_cast<vtkHigherOrderHexahedron*>(cell->GetRepresentativeCell());
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1], degs[2]);
      }
      else
      {
        vtkIdType numPts = cell->PointIds->GetNumberOfIds();
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }
      break;
    }

    // Set the degree and rational weights for Bezier elements
    case VTK_BEZIER_QUADRILATERAL:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierQuadrilateral* cellBezier =
        dynamic_cast<vtkBezierQuadrilateral*>(cell->GetRepresentativeCell());

      // Set the degrees
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1]);
      }
      else
      {
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }

      // Set the weights
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }
    case VTK_BEZIER_HEXAHEDRON:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierHexahedron* cellBezier =
        dynamic_cast<vtkBezierHexahedron*>(cell->GetRepresentativeCell());

      // Set the degrees
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1], degs[2]);
      }
      else
      {
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }

      // Set the weights
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }
    case VTK_BEZIER_WEDGE:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierWedge* cellBezier = dynamic_cast<vtkBezierWedge*>(cell->GetRepresentativeCell());

      // Set the degrees
      vtkDataArray* v = this->GetCellData()->GetHigherOrderDegrees();
      if (v)
      {
        double degs[3];
        v->GetTuple(cellId, degs);
        cellBezier->SetOrder(degs[0], degs[1], degs[2], numPts);
      }
      else
      {
        cellBezier->SetUniformOrderFromNumPoints(numPts);
      }

      // Set the weights
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }

    case VTK_BEZIER_CURVE:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierCurve* cellBezier = dynamic_cast<vtkBezierCurve*>(cell->GetRepresentativeCell());
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }
    case VTK_BEZIER_TRIANGLE:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierTriangle* cellBezier =
        dynamic_cast<vtkBezierTriangle*>(cell->GetRepresentativeCell());
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }
    case VTK_BEZIER_TETRAHEDRON:
    {
      vtkIdType numPts = cell->PointIds->GetNumberOfIds();
      vtkBezierTetra* cellBezier = dynamic_cast<vtkBezierTetra*>(cell->GetRepresentativeCell());
      cellBezier->SetRationalWeightsFromPointData(GetPointData(), numPts);
      break;
    }
    default:
      break;
  }
}

//------------------------------------------------------------------------------
// Default implementation. This is very slow way to compute this information.
// Subclasses should override this method for efficiency.
void vtkDataSet::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  vtkNew<vtkGenericCell> cell;
  this->GetCell(cellId, cell);
  cell->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkDataSet::Squeeze()
{
  this->CellData->Squeeze();
  this->PointData->Squeeze();
}

//------------------------------------------------------------------------------
unsigned long vtkDataSet::GetActualMemorySize()
{
  unsigned long size = this->vtkDataObject::GetActualMemorySize();
  size += this->PointData->GetActualMemorySize();
  size += this->CellData->GetActualMemorySize();
  return size;
}

//------------------------------------------------------------------------------
void vtkDataSet::ShallowCopy(vtkDataObject* dataObject)
{
  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(dataObject);

  if (dataSet != nullptr)
  {
    this->InternalDataSetCopy(dataSet);
    this->CellData->ShallowCopy(dataSet->GetCellData());
    this->PointData->ShallowCopy(dataSet->GetPointData());
  }
  // Do superclass
  this->vtkDataObject::ShallowCopy(dataObject);
}

//------------------------------------------------------------------------------
void vtkDataSet::DeepCopy(vtkDataObject* dataObject)
{
  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(dataObject);

  if (dataSet != nullptr)
  {
    this->InternalDataSetCopy(dataSet);
    this->CellData->DeepCopy(dataSet->GetCellData());
    this->PointData->DeepCopy(dataSet->GetPointData());
  }

  // Do superclass
  this->vtkDataObject::DeepCopy(dataObject);
}

//------------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkDataSet::InternalDataSetCopy(vtkDataSet* src)
{
  int idx;

  this->ScalarRangeComputeTime = src->ScalarRangeComputeTime;
  this->ScalarRange[0] = src->ScalarRange[0];
  this->ScalarRange[1] = src->ScalarRange[1];

  this->ComputeTime = src->ComputeTime;
  for (idx = 0; idx < 3; ++idx)
  {
    this->Bounds[2 * idx] = src->Bounds[2 * idx];
    this->Bounds[2 * idx + 1] = src->Bounds[2 * idx + 1];
  }
}

//------------------------------------------------------------------------------
int vtkDataSet::CheckAttributes()
{
  vtkIdType numPts, numCells;
  int numArrays, idx;
  vtkAbstractArray* array;
  vtkIdType numTuples;
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
      if (name == nullptr)
      {
        name = "";
      }
      if (numTuples < numPts)
      {
        vtkErrorMacro("Point array " << name << " with " << array->GetNumberOfComponents()
                                     << " components, only has " << numTuples
                                     << " tuples but there are " << numPts << " points");
        return 1;
      }
      if (numTuples > numPts)
      {
        vtkWarningMacro("Point array " << name << " with " << array->GetNumberOfComponents()
                                       << " components, has " << numTuples
                                       << " tuples but there are only " << numPts << " points");
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
      if (name == nullptr)
      {
        name = "";
      }
      if (numTuples < numCells)
      {
        vtkErrorMacro("Cell array " << name << " with " << array->GetNumberOfComponents()
                                    << " components, has only " << numTuples
                                    << " tuples but there are " << numCells << " cells");
        return 1;
      }
      if (numTuples > numCells)
      {
        vtkWarningMacro("Cell array " << name << " with " << array->GetNumberOfComponents()
                                      << " components, has " << numTuples
                                      << " tuples but there are only " << numCells << " cells");
      }
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
void vtkDataSet::GenerateGhostArray(int zeroExt[6], bool cellOnly)
{
  // Make sure this is a structured data set.
  if (this->GetExtentType() != VTK_3D_EXTENT)
  {
    return;
  }

  int extent[6];
  this->Information->Get(vtkDataObject::DATA_EXTENT(), extent);

  int i, j, k, di, dj, dk, dist;

  bool sameExtent = true;
  for (i = 0; i < 6; i++)
  {
    if (extent[i] != zeroExt[i])
    {
      sameExtent = false;
      break;
    }
  }
  if (sameExtent)
  {
    return;
  }

  vtkIdType index = 0;

  // ---- POINTS ----

  if (!cellOnly)
  {
    vtkSmartPointer<vtkUnsignedCharArray> ghostPoints = vtkArrayDownCast<vtkUnsignedCharArray>(
      this->PointData->GetArray(vtkDataSetAttributes::GhostArrayName()));
    if (!ghostPoints)
    {
      ghostPoints.TakeReference(vtkUnsignedCharArray::New());
      ghostPoints->SetName(vtkDataSetAttributes::GhostArrayName());
      ghostPoints->SetNumberOfValues(vtkStructuredData::GetNumberOfPoints(extent));
      ghostPoints->FillValue(0);
      this->PointData->AddArray(ghostPoints);
    }

    // Loop through the points in this image.
    for (k = extent[4]; k <= extent[5]; ++k)
    {
      dk = 0;
      if (k < zeroExt[4])
      {
        dk = zeroExt[4] - k;
      }
      if (k > zeroExt[5])
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
        if (j > zeroExt[3])
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
          if (i > zeroExt[1])
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
          unsigned char value = ghostPoints->GetValue(index);
          if (dist > 0)
          {
            value |= vtkDataSetAttributes::DUPLICATEPOINT;
          }
          ghostPoints->SetValue(index, value);
          index++;
        }
      }
    }
  }

  // ---- CELLS ----

  vtkSmartPointer<vtkUnsignedCharArray> ghostCells = vtkArrayDownCast<vtkUnsignedCharArray>(
    this->CellData->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (!ghostCells)
  {
    ghostCells.TakeReference(vtkUnsignedCharArray::New());
    ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostCells->SetNumberOfValues(vtkStructuredData::GetNumberOfCells(extent));
    ghostCells->FillValue(0);
    this->CellData->AddArray(ghostCells);
  }

  index = 0;

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
        unsigned char value = ghostCells->GetValue(index);
        if (dist > 0)
        {
          value |= vtkDataSetAttributes::DUPLICATECELL;
        }
        ghostCells->SetValue(index, value);
        index++;
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkDataSet* vtkDataSet::GetData(vtkInformation* info)
{
  return info ? vtkDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : nullptr;
}

//------------------------------------------------------------------------------
vtkDataSet* vtkDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkDataSet::GetData(v->GetInformationObject(i));
}

//------------------------------------------------------------------------------
vtkFieldData* vtkDataSet::GetAttributesAsFieldData(int type)
{
  switch (type)
  {
    case POINT:
      return this->GetPointData();
    case CELL:
      return this->GetCellData();
  }
  return this->Superclass::GetAttributesAsFieldData(type);
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSet::GetNumberOfElements(int type)
{
  switch (type)
  {
    case POINT:
      return this->GetNumberOfPoints();
    case CELL:
      return this->GetNumberOfCells();
  }
  return this->Superclass::GetNumberOfElements(type);
}

//------------------------------------------------------------------------------
vtkIdType vtkDataSet::GetCellSize(vtkIdType cellId)
{
  // We allocate a new id list each time so this method is thread-safe
  vtkNew<vtkIdList> pointIds;
  this->GetCellPoints(cellId, pointIds);
  return pointIds->GetNumberOfIds();
}

//------------------------------------------------------------------------------
void vtkDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  os << indent << "Number Of Cells: " << this->GetNumberOfCells() << "\n";

  os << indent << "Cell Data:\n";
  this->CellData->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point Data:\n";
  this->PointData->PrintSelf(os, indent.GetNextIndent());

  const double* bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
  os << indent << "Compute Time: " << this->ComputeTime.GetMTime() << "\n";
}

//------------------------------------------------------------------------------
bool vtkDataSet::HasAnyGhostPoints()
{
  return this->PointData->HasAnyGhostBitSet(vtkDataSetAttributes::DUPLICATEPOINT);
}

//------------------------------------------------------------------------------
bool vtkDataSet::HasAnyGhostCells()
{
  return this->CellData->HasAnyGhostBitSet(vtkDataSetAttributes::DUPLICATECELL);
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDataSet::GetPointGhostArray()
{
  return this->PointData->GetGhostArray();
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDataSet::GetGhostArray(int attributeType)
{
  if (attributeType == POINT)
  {
    return this->GetPointGhostArray();
  }
  else if (attributeType == CELL)
  {
    return this->GetCellGhostArray();
  }
  else
  {
    vtkErrorMacro("Invalid attribute type for ghost arrays: " << attributeType);
    return nullptr;
  }
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDataSet::AllocatePointGhostArray()
{
  if (!this->GetPointGhostArray())
  {
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfValues(this->GetNumberOfPoints());
    ghosts->FillValue(0);
    this->GetPointData()->AddArray(ghosts);
  }
  return this->GetPointGhostArray();
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDataSet::GetCellGhostArray()
{
  return this->CellData->GetGhostArray();
}

//------------------------------------------------------------------------------
vtkUnsignedCharArray* vtkDataSet::AllocateCellGhostArray()
{
  if (!this->GetCellGhostArray())
  {
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfValues(this->GetNumberOfCells());
    ghosts->FillValue(0);
    this->GetCellData()->AddArray(ghosts);
  }
  return this->GetCellGhostArray();
}

//------------------------------------------------------------------------------
void vtkDataSet::OnDataModified(vtkObject* source, unsigned long, void* clientdata, void*)
{
  // update the point/cell pointers to ghost data arrays.
  vtkDataSet* This = static_cast<vtkDataSet*>(clientdata);
  if (source == This->GetPointData())
  {
    This->UpdatePointGhostArrayCache();
  }
  else
  {
    assert(source == This->GetCellData());
    This->UpdateCellGhostArrayCache();
  }
}
VTK_ABI_NAMESPACE_END
