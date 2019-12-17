//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include "vtkmDataSet.h"

#include "vtkmDataArray.h"
#include "vtkmFilterPolicy.h"
#include "vtkmlib/ArrayConverters.h"

#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"

#include <vtkm/List.h>
#include <vtkm/cont/CellLocatorGeneral.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/PointLocator.h>
#include <vtkm/cont/PointLocatorUniformGrid.h>
#include <vtkm/worklet/ScatterPermutation.h>

#include <mutex>

namespace
{

using SupportedCellSets =
  vtkm::ListAppend<vtkmInputFilterPolicy::AllCellSetList, vtkmOutputFilterPolicy::AllCellSetList>;

template <typename LocatorControl>
struct VtkmLocator
{
  std::mutex lock;
  std::unique_ptr<LocatorControl> control;
  vtkMTimeType buildTime = 0;
};

} // anonymous

struct vtkmDataSet::DataMembers
{
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkNew<vtkGenericCell> Cell;

  VtkmLocator<vtkm::cont::PointLocator> PointLocator;
  VtkmLocator<vtkm::cont::CellLocator> CellLocator;
};

//----------------------------------------------------------------------------
vtkmDataSet::vtkmDataSet()
  : Internals(new DataMembers)
{
}

vtkmDataSet::~vtkmDataSet() = default;

vtkStandardNewMacro(vtkmDataSet);

void vtkmDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Internals->CellSet.PrintSummary(os);
  this->Internals->Coordinates.PrintSummary(os);
}

//----------------------------------------------------------------------------
void vtkmDataSet::SetVtkmDataSet(const vtkm::cont::DataSet& ds)
{
  this->Internals->CellSet = ds.GetCellSet();
  this->Internals->Coordinates = ds.GetCoordinateSystem();
  fromvtkm::ConvertArrays(ds, this);
}

vtkm::cont::DataSet vtkmDataSet::GetVtkmDataSet() const
{
  vtkm::cont::DataSet ds;
  ds.SetCellSet(this->Internals->CellSet);
  ds.AddCoordinateSystem(this->Internals->Coordinates);
  tovtkm::ProcessFields(const_cast<vtkmDataSet*>(this), ds, tovtkm::FieldsFlag::PointsAndCells);

  return ds;
}

//----------------------------------------------------------------------------
void vtkmDataSet::CopyStructure(vtkDataSet* ds)
{
  auto vtkmds = vtkmDataSet::SafeDownCast(ds);
  if (vtkmds)
  {
    this->Initialize();
    this->Internals->CellSet = vtkmds->Internals->CellSet;
    this->Internals->Coordinates = vtkmds->Internals->Coordinates;
  }
}

vtkIdType vtkmDataSet::GetNumberOfPoints()
{
  return this->Internals->Coordinates.GetNumberOfPoints();
}

vtkIdType vtkmDataSet::GetNumberOfCells()
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  return csBase ? csBase->GetNumberOfCells() : 0;
}

double* vtkmDataSet::GetPoint(vtkIdType ptId)
{
  static double point[3];
  this->GetPoint(ptId, point);
  return point;
}

void vtkmDataSet::GetPoint(vtkIdType id, double x[3])
{
  auto portal = this->Internals->Coordinates.GetData().GetPortalConstControl();
  auto value = portal.Get(id);
  x[0] = value[0];
  x[1] = value[1];
  x[2] = value[2];
}

vtkCell* vtkmDataSet::GetCell(vtkIdType cellId)
{
  this->GetCell(cellId, this->Internals->Cell);
  return this->Internals->Cell->GetRepresentativeCell();
}

void vtkmDataSet::GetCell(vtkIdType cellId, vtkGenericCell* cell)
{
  cell->SetCellType(this->GetCellType(cellId));

  auto* pointIds = cell->GetPointIds();
  this->GetCellPoints(cellId, pointIds);

  auto numPoints = pointIds->GetNumberOfIds();
  cell->GetPoints()->SetNumberOfPoints(numPoints);
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double x[3];
    this->GetPoint(pointIds->GetId(i), x);
    cell->GetPoints()->SetPoint(i, x);
  }
}

void vtkmDataSet::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  if (this->Internals->Coordinates.GetData()
        .IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>() &&
    this->Internals->CellSet.IsType<vtkm::cont::CellSetStructured<3> >())
  {
    auto portal = this->Internals->Coordinates.GetData()
                    .Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>()
                    .GetPortalConstControl();

    vtkm::internal::ConnectivityStructuredInternals<3> helper;
    helper.SetPointDimensions(portal.GetDimensions());
    auto id3 = helper.FlatToLogicalCellIndex(cellId);
    auto min = portal.Get(id3);
    auto max = min + portal.GetSpacing();
    for (int i = 0; i < 3; ++i)
    {
      bounds[2 * i] = min[i];
      bounds[2 * i + 1] = max[i];
    }
  }
  else
  {
    Superclass::GetCellBounds(cellId, bounds);
  }
}

int vtkmDataSet::GetCellType(vtkIdType cellId)
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  if (csBase)
  {
    return csBase->GetCellShape(cellId);
  }
  return VTK_EMPTY_CELL;
}

void vtkmDataSet::GetCellPoints(vtkIdType cellId, vtkIdList* ptIds)
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  if (csBase)
  {
    vtkm::Id numPoints = csBase->GetNumberOfPointsInCell(cellId);
    ptIds->SetNumberOfIds(numPoints);
    csBase->GetCellPointIds(cellId, ptIds->GetPointer(0));
  }
}

namespace
{

struct WorkletGetPointCells : vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn);
  using ExecutionSignature = void(CellCount, CellIndices, Device);
  using ScatterType = vtkm::worklet::ScatterPermutation<>;

  explicit WorkletGetPointCells(vtkIdList* output)
    : Output(output)
  {
  }

  template <typename IndicesVecType>
  VTKM_EXEC void operator()(vtkm::Id, IndicesVecType, vtkm::cont::DeviceAdapterTagCuda) const
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename IndicesVecType, typename Device>
  VTKM_EXEC void operator()(vtkm::Id count, IndicesVecType idxs, Device) const
  {
    this->Output->SetNumberOfIds(count);
    for (vtkm::Id i = 0; i < count; ++i)
    {
      this->Output->SetId(i, idxs[i]);
    }
  }

  vtkIdList* Output;
};

} // anonymous namespace

void vtkmDataSet::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  auto scatter = WorkletGetPointCells::ScatterType(vtkm::cont::make_ArrayHandle(&ptId, 1));
  vtkm::cont::Invoker invoke(vtkm::cont::DeviceAdapterTagSerial{});
  invoke(WorkletGetPointCells{ cellIds }, scatter,
    this->Internals->CellSet.ResetCellSetList(SupportedCellSets{}));
}

vtkIdType vtkmDataSet::FindPoint(double x[3])
{
  auto& locator = this->Internals->PointLocator;
  // critical section
  {
    std::lock_guard<std::mutex> lock(locator.lock);
    if (locator.buildTime < this->GetMTime())
    {
      locator.control.reset(new vtkm::cont::PointLocatorUniformGrid);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  auto execLocator = locator.control->PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial{});

  vtkm::Vec<vtkm::FloatDefault, 3> point(x[0], x[1], x[2]);
  vtkm::Id pointId = -1;
  vtkm::FloatDefault d2 = 0;
  // exec object created for the Serial device can be called directly
  execLocator->FindNearestNeighbor(point, pointId, d2);
  return pointId;
}

// non thread-safe version
vtkIdType vtkmDataSet::FindCell(
  double x[3], vtkCell*, vtkIdType, double, int& subId, double pcoords[3], double* weights)
{
  // just call the thread-safe version
  return this->FindCell(x, nullptr, nullptr, -1, 0.0, subId, pcoords, weights);
}

// thread-safe version
vtkIdType vtkmDataSet::FindCell(double x[3], vtkCell*, vtkGenericCell*, vtkIdType, double,
  int& subId, double pcoords[3], double* weights)
{
  auto& locator = this->Internals->CellLocator;
  // critical section
  {
    std::lock_guard<std::mutex> lock(locator.lock);
    if (locator.buildTime < this->GetMTime())
    {
      locator.control.reset(new vtkm::cont::CellLocatorGeneral);
      locator.control->SetCellSet(this->Internals->CellSet);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  auto execLocator = locator.control->PrepareForExecution(vtkm::cont::DeviceAdapterTagSerial{});

  vtkm::Vec<vtkm::FloatDefault, 3> point(x[0], x[1], x[2]);
  vtkm::Vec<vtkm::FloatDefault, 3> pc;
  vtkm::Id cellId = -1;
  // exec object created for the Serial device can be called directly
  execLocator->FindCell(point, cellId, pc, vtkm::worklet::WorkletMapField{});

  if (cellId >= 0)
  {
    double closestPoint[3], dist2;
    vtkNew<vtkGenericCell> vtkcell;
    this->GetCell(cellId, vtkcell);
    vtkcell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
  }

  return cellId;
}

void vtkmDataSet::Squeeze()
{
  Superclass::Squeeze();

  this->Internals->PointLocator.control.reset(nullptr);
  this->Internals->PointLocator.buildTime = 0;
  this->Internals->CellLocator.control.reset(nullptr);
  this->Internals->CellLocator.buildTime = 0;
}

void vtkmDataSet::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
  {
    vtkm::Bounds bounds = this->Internals->Coordinates.GetBounds();
    this->Bounds[0] = bounds.X.Min;
    this->Bounds[1] = bounds.X.Max;
    this->Bounds[2] = bounds.Y.Min;
    this->Bounds[3] = bounds.Y.Max;
    this->Bounds[4] = bounds.Z.Min;
    this->Bounds[5] = bounds.Z.Max;
    this->ComputeTime.Modified();
  }
}

void vtkmDataSet::Initialize()
{
  Superclass::Initialize();
  this->Internals = std::make_shared<DataMembers>();
}

namespace
{

struct MaxCellSize
{
  template <vtkm::IdComponent DIM>
  void operator()(
    const vtkm::cont::CellSetStructured<DIM>& cellset, vtkm::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S>
  void operator()(const vtkm::cont::CellSetSingleType<S>& cellset, vtkm::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S1, typename S2, typename S3>
  void operator()(
    const vtkm::cont::CellSetExplicit<S1, S2, S3>& cellset, vtkm::IdComponent& result) const
  {
    auto counts =
      cellset.GetNumIndicesArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
    result = vtkm::cont::Algorithm::Reduce(counts, vtkm::IdComponent{ 0 }, vtkm::Maximum{});
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset, vtkm::IdComponent& result) const
  {
    result = -1;
    vtkm::Id numberOfCells = cellset.GetNumberOfCells();
    for (vtkm::Id i = 0; i < numberOfCells; ++i)
    {
      result = std::max(result, cellset.GetNumberOfPointsInCell(i));
    }
  }
};
} // anonymous namespace

int vtkmDataSet::GetMaxCellSize()
{
  vtkm::IdComponent result = 0;
  vtkm::cont::CastAndCall(
    this->Internals->CellSet.ResetCellSetList(SupportedCellSets{}), MaxCellSize{}, result);
  return result;
}

unsigned long vtkmDataSet::GetActualMemorySize()
{
  return this->Superclass::GetActualMemorySize();
}

void vtkmDataSet::ShallowCopy(vtkDataObject* src)
{
  auto obj = vtkmDataSet::SafeDownCast(src);
  if (obj)
  {
    Superclass::ShallowCopy(obj);
    this->Internals = obj->Internals;
  }
}

void vtkmDataSet::DeepCopy(vtkDataObject* src)
{
  auto other = vtkmDataSet::SafeDownCast(src);
  if (other)
  {
    auto* csBase = other->Internals->CellSet.GetCellSetBase();
    if (csBase)
    {
      this->Initialize();

      this->Internals->CellSet = other->Internals->CellSet.NewInstance();
      this->Internals->CellSet.GetCellSetBase()->DeepCopy(csBase);
    }
  }
}
