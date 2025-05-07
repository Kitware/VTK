// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2015 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2015 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2015 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov
#include "vtkmDataSet.h"

#include "vtkmDataArray.h"
#include "vtkmlib/ArrayConverters.h"

#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"

#include <viskores/List.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellLocatorGeneral.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetPermutation.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DefaultTypes.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/PointLocatorSparseGrid.h>
#include <viskores/worklet/ScatterPermutation.h>

#include <mutex>

VTK_ABI_NAMESPACE_BEGIN
namespace
{

using SupportedCellSets = toviskores::CellListAllOutVTK;

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
  viskores::cont::UnknownCellSet CellSet;
  viskores::cont::CoordinateSystem Coordinates;
  vtkNew<vtkGenericCell> Cell;

  VtkmLocator<viskores::cont::PointLocatorSparseGrid> PointLocator;
  VtkmLocator<viskores::cont::CellLocatorGeneral> CellLocator;
};

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkmDataSet::SetVtkmDataSet(const viskores::cont::DataSet& ds)
{
  this->Internals->CellSet = ds.GetCellSet();
  this->Internals->Coordinates = ds.GetCoordinateSystem();
  fromvtkm::ConvertArrays(ds, this);
}

viskores::cont::DataSet vtkmDataSet::GetVtkmDataSet() const
{
  viskores::cont::DataSet ds;
  ds.SetCellSet(this->Internals->CellSet);
  ds.AddCoordinateSystem(this->Internals->Coordinates);
  tovtkm::ProcessFields(const_cast<vtkmDataSet*>(this), ds, tovtkm::FieldsFlag::PointsAndCells);

  return ds;
}

//------------------------------------------------------------------------------
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
  auto pointArray = this->Internals->Coordinates.GetDataAsMultiplexer();
  auto portal = pointArray.ReadPortal();
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
        .IsType<viskores::cont::ArrayHandleUniformPointCoordinates>() &&
    this->Internals->CellSet.IsType<viskores::cont::CellSetStructured<3>>())
  {
    auto portal = this->Internals->Coordinates.GetData()
                    .AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>()
                    .ReadPortal();

    viskores::internal::ConnectivityStructuredInternals<3> helper;
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
    viskores::Id numPoints = csBase->GetNumberOfPointsInCell(cellId);
    ptIds->SetNumberOfIds(numPoints);
    csBase->GetCellPointIds(cellId, ptIds->GetPointer(0));
  }
}

namespace
{

struct WorkletGetPointCells : viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn);
  using ExecutionSignature = void(CellCount, CellIndices, Device);
  using ScatterType = viskores::worklet::ScatterPermutation<>;

  explicit WorkletGetPointCells(vtkIdList* output)
    : Output(output)
  {
  }

  template <typename IndicesVecType, typename Device>
  VISKORES_EXEC void operator()(viskores::Id, IndicesVecType, Device) const
  {
    this->RaiseError("This worklet should only be called on serial device");
  }

  // This method is declared VISKORES_CONT because we have set it to only
  // run on the serial device (see the third argument). Declaring it
  // as VISKORES_CONT will prevent compiler warnings/errors about calling
  // a host function from a device that can never happen.
  template <typename IndicesVecType>
  VISKORES_CONT void operator()(
    viskores::Id count, IndicesVecType idxs, viskores::cont::DeviceAdapterTagSerial) const
  {
    this->Output->SetNumberOfIds(count);
    for (viskores::Id i = 0; i < count; ++i)
    {
      this->Output->SetId(i, idxs[i]);
    }
  }

  vtkIdList* Output;
};

} // anonymous namespace

void vtkmDataSet::GetPointCells(vtkIdType ptId, vtkIdList* cellIds)
{
  auto scatter = WorkletGetPointCells::ScatterType(
    viskores::cont::make_ArrayHandle(&ptId, 1, viskores::CopyFlag::Off));
  viskores::cont::Invoker invoke(viskores::cont::DeviceAdapterTagSerial{});
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
      locator.control.reset(new viskores::cont::PointLocatorSparseGrid);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  viskores::cont::Token token;
  auto execLocator =
    locator.control->PrepareForExecution(viskores::cont::DeviceAdapterTagSerial{}, token);

  viskores::Vec<viskores::FloatDefault, 3> point(x[0], x[1], x[2]);
  viskores::Id pointId = -1;
  viskores::FloatDefault d2 = 0;
  // exec object created for the Serial device can be called directly
  execLocator.FindNearestNeighbor(point, pointId, d2);
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
      locator.control.reset(new viskores::cont::CellLocatorGeneral);
      locator.control->SetCellSet(this->Internals->CellSet);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  viskores::cont::Token token;
  auto execLocator =
    locator.control->PrepareForExecution(viskores::cont::DeviceAdapterTagSerial{}, token);

  viskores::Vec<viskores::FloatDefault, 3> point(x[0], x[1], x[2]);
  viskores::Vec<viskores::FloatDefault, 3> pc;
  viskores::Id cellId = -1;
  // exec object created for the Serial device can be called directly
  execLocator.FindCell(point, cellId, pc);

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
    viskores::Bounds bounds = this->Internals->Coordinates.GetBounds();
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
  template <viskores::IdComponent DIM>
  void operator()(
    const viskores::cont::CellSetStructured<DIM>& cellset, viskores::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S>
  void operator()(
    const viskores::cont::CellSetSingleType<S>& cellset, viskores::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S1, typename S2, typename S3>
  void operator()(
    const viskores::cont::CellSetExplicit<S1, S2, S3>& cellset, viskores::IdComponent& result) const
  {
    auto counts = cellset.GetNumIndicesArray(
      viskores::TopologyElementTagCell{}, viskores::TopologyElementTagPoint{});
    result =
      viskores::cont::Algorithm::Reduce(counts, viskores::IdComponent{ 0 }, viskores::Maximum{});
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset, viskores::IdComponent& result) const
  {
    result = -1;
    viskores::Id numberOfCells = cellset.GetNumberOfCells();
    for (viskores::Id i = 0; i < numberOfCells; ++i)
    {
      result = std::max(result, cellset.GetNumberOfPointsInCell(i));
    }
  }
};
} // anonymous namespace

int vtkmDataSet::GetMaxCellSize()
{
  viskores::IdComponent result = 0;
  viskores::cont::CastAndCall(
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
VTK_ABI_NAMESPACE_END
