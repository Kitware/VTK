//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/CellSetExtrude.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/CellShape.h>

namespace viskores
{
namespace cont
{

CellSetExtrude::CellSetExtrude()
  : viskores::cont::CellSet()
  , IsPeriodic(false)
  , NumberOfPointsPerPlane(0)
  , NumberOfCellsPerPlane(0)
  , NumberOfPlanes(0)
  , ReverseConnectivityBuilt(false)
{
}

CellSetExtrude::CellSetExtrude(const viskores::cont::ArrayHandle<viskores::Int32>& conn,
                               viskores::Int32 numberOfPointsPerPlane,
                               viskores::Int32 numberOfPlanes,
                               const viskores::cont::ArrayHandle<viskores::Int32>& nextNode,
                               bool periodic)
  : viskores::cont::CellSet()
  , IsPeriodic(periodic)
  , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  , NumberOfCellsPerPlane(
      static_cast<viskores::Int32>(conn.GetNumberOfValues() / static_cast<viskores::Id>(3)))
  , NumberOfPlanes(numberOfPlanes)
  , Connectivity(conn)
  , NextNode(nextNode)
  , ReverseConnectivityBuilt(false)
{
}


CellSetExtrude::CellSetExtrude(const CellSetExtrude& src)
  : CellSet(src)
  , IsPeriodic(src.IsPeriodic)
  , NumberOfPointsPerPlane(src.NumberOfPointsPerPlane)
  , NumberOfCellsPerPlane(src.NumberOfCellsPerPlane)
  , NumberOfPlanes(src.NumberOfPlanes)
  , Connectivity(src.Connectivity)
  , NextNode(src.NextNode)
  , ReverseConnectivityBuilt(src.ReverseConnectivityBuilt)
  , RConnectivity(src.RConnectivity)
  , ROffsets(src.ROffsets)
  , RCounts(src.RCounts)
  , PrevNode(src.PrevNode)
{
}

CellSetExtrude::CellSetExtrude(CellSetExtrude&& src) noexcept
  : CellSet(std::forward<CellSet>(src))
  , IsPeriodic(src.IsPeriodic)
  , NumberOfPointsPerPlane(src.NumberOfPointsPerPlane)
  , NumberOfCellsPerPlane(src.NumberOfCellsPerPlane)
  , NumberOfPlanes(src.NumberOfPlanes)
  , Connectivity(std::move(src.Connectivity))
  , NextNode(std::move(src.NextNode))
  , ReverseConnectivityBuilt(src.ReverseConnectivityBuilt)
  , RConnectivity(std::move(src.RConnectivity))
  , ROffsets(std::move(src.ROffsets))
  , RCounts(std::move(src.RCounts))
  , PrevNode(std::move(src.PrevNode))
{
}

CellSetExtrude& CellSetExtrude::operator=(const CellSetExtrude& src)
{
  this->CellSet::operator=(src);

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = src.Connectivity;
  this->NextNode = src.NextNode;
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = src.RConnectivity;
  this->ROffsets = src.ROffsets;
  this->RCounts = src.RCounts;
  this->PrevNode = src.PrevNode;

  return *this;
}

CellSetExtrude& CellSetExtrude::operator=(CellSetExtrude&& src) noexcept
{
  this->CellSet::operator=(std::forward<CellSet>(src));

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = std::move(src.Connectivity);
  this->NextNode = std::move(src.NextNode);
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = std::move(src.RConnectivity);
  this->ROffsets = std::move(src.ROffsets);
  this->RCounts = std::move(src.RCounts);
  this->PrevNode = std::move(src.PrevNode);

  return *this;
}

CellSetExtrude::~CellSetExtrude() {}

viskores::Int32 CellSetExtrude::GetNumberOfPlanes() const
{
  return this->NumberOfPlanes;
}

viskores::Id CellSetExtrude::GetNumberOfCells() const
{
  if (this->IsPeriodic)
  {
    return static_cast<viskores::Id>(this->NumberOfPlanes) *
      static_cast<viskores::Id>(this->NumberOfCellsPerPlane);
  }
  else
  {
    return static_cast<viskores::Id>(this->NumberOfPlanes - 1) *
      static_cast<viskores::Id>(this->NumberOfCellsPerPlane);
  }
}

viskores::Id CellSetExtrude::GetNumberOfPoints() const
{
  return static_cast<viskores::Id>(this->NumberOfPlanes) *
    static_cast<viskores::Id>(this->NumberOfPointsPerPlane);
}

viskores::Id CellSetExtrude::GetNumberOfFaces() const
{
  return -1;
}

viskores::Id CellSetExtrude::GetNumberOfEdges() const
{
  return -1;
}

viskores::UInt8 CellSetExtrude::GetCellShape(viskores::Id) const
{
  return viskores::CellShapeTagWedge::Id;
}

viskores::IdComponent CellSetExtrude::GetNumberOfPointsInCell(viskores::Id) const
{
  return 6;
}

void CellSetExtrude::GetCellPointIds(viskores::Id id, viskores::Id* ptids) const
{
  viskores::cont::Token token;
  auto conn = this->PrepareForInput(viskores::cont::DeviceAdapterTagSerial{},
                                    viskores::TopologyElementTagCell{},
                                    viskores::TopologyElementTagPoint{},
                                    token);
  auto indices = conn.GetIndices(id);
  for (int i = 0; i < 6; ++i)
  {
    ptids[i] = indices[i];
  }
}

template <viskores::IdComponent NumIndices>
VISKORES_CONT void CellSetExtrude::GetIndices(viskores::Id index,
                                              viskores::Vec<viskores::Id, NumIndices>& ids) const
{
  static_assert(NumIndices == 6, "There are always 6 points in a wedge.");
  this->GetCellPointIds(index, ids.data());
}

VISKORES_CONT void CellSetExtrude::GetIndices(viskores::Id index,
                                              viskores::cont::ArrayHandle<viskores::Id>& ids) const
{
  ids.Allocate(6);
  auto outIdPortal = ids.WritePortal();
  viskores::cont::Token token;
  auto conn = this->PrepareForInput(viskores::cont::DeviceAdapterTagSerial{},
                                    viskores::TopologyElementTagCell{},
                                    viskores::TopologyElementTagPoint{},
                                    token);
  auto indices = conn.GetIndices(index);
  for (viskores::IdComponent i = 0; i < 6; i++)
  {
    outIdPortal.Set(i, indices[i]);
  }
}

std::shared_ptr<CellSet> CellSetExtrude::NewInstance() const
{
  return std::make_shared<CellSetExtrude>();
}

void CellSetExtrude::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExtrude*>(src);
  if (!other)
  {
    throw viskores::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  this->IsPeriodic = other->IsPeriodic;

  this->NumberOfPointsPerPlane = other->NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = other->NumberOfCellsPerPlane;
  this->NumberOfPlanes = other->NumberOfPlanes;

  viskores::cont::ArrayCopy(other->Connectivity, this->Connectivity);
  viskores::cont::ArrayCopy(other->NextNode, this->NextNode);

  this->ReverseConnectivityBuilt = other->ReverseConnectivityBuilt;

  if (this->ReverseConnectivityBuilt)
  {
    viskores::cont::ArrayCopy(other->RConnectivity, this->RConnectivity);
    viskores::cont::ArrayCopy(other->ROffsets, this->ROffsets);
    viskores::cont::ArrayCopy(other->RCounts, this->RCounts);
    viskores::cont::ArrayCopy(other->PrevNode, this->PrevNode);
  }
}

void CellSetExtrude::ReleaseResourcesExecution()
{
  this->Connectivity.ReleaseResourcesExecution();
  this->NextNode.ReleaseResourcesExecution();

  this->RConnectivity.ReleaseResourcesExecution();
  this->ROffsets.ReleaseResourcesExecution();
  this->RCounts.ReleaseResourcesExecution();
  this->PrevNode.ReleaseResourcesExecution();
}

viskores::Id2 CellSetExtrude::GetSchedulingRange(viskores::TopologyElementTagCell) const
{
  if (this->IsPeriodic)
  {
    return viskores::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes);
  }
  else
  {
    return viskores::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes - 1);
  }
}

viskores::Id2 CellSetExtrude::GetSchedulingRange(viskores::TopologyElementTagPoint) const
{
  return viskores::Id2(this->NumberOfPointsPerPlane, this->NumberOfPlanes);
}

void CellSetExtrude::PrintSummary(std::ostream& out) const
{
  out << "   viskoresCellSetSingleType: " << std::endl;
  out << "   NumberOfCellsPerPlane: " << this->NumberOfCellsPerPlane << std::endl;
  out << "   NumberOfPointsPerPlane: " << this->NumberOfPointsPerPlane << std::endl;
  out << "   NumberOfPlanes: " << this->NumberOfPlanes << std::endl;
  out << "   Connectivity: " << std::endl;
  viskores::cont::printSummary_ArrayHandle(this->Connectivity, out);
  out << "   NextNode: " << std::endl;
  viskores::cont::printSummary_ArrayHandle(this->NextNode, out);
  out << "   ReverseConnectivityBuilt: " << this->NumberOfPlanes << std::endl;
}


namespace
{

struct ComputeReverseMapping : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn cellIndex, WholeArrayOut cellIds);

  VISKORES_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VISKORES_EXEC void operator()(viskores::Id cellId, PortalType&& pointIdValue) const
  {
    //3 as we are building the connectivity for triangles
    const viskores::Id offset = 3 * cellId;
    pointIdValue.Set(offset, static_cast<viskores::Int32>(cellId));
    pointIdValue.Set(offset + 1, static_cast<viskores::Int32>(cellId));
    pointIdValue.Set(offset + 2, static_cast<viskores::Int32>(cellId));
  }
};

struct ComputePrevNode : public viskores::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nextNode, WholeArrayOut prevNodeArray);
  typedef void ExecutionSignature(InputIndex, _1, _2);

  template <typename PortalType>
  VISKORES_EXEC void operator()(viskores::Id idx, viskores::Int32 next, PortalType& prevs) const
  {
    prevs.Set(static_cast<viskores::Id>(next), static_cast<viskores::Int32>(idx));
  }
};

} // anonymous namespace

VISKORES_CONT void CellSetExtrude::BuildReverseConnectivity()
{
  viskores::cont::Invoker invoke;

  // create a mapping of where each key is the point id and the value
  // is the cell id. We
  const viskores::Id numberOfPointsPerCell = 3;
  const viskores::Id rconnSize = this->NumberOfCellsPerPlane * numberOfPointsPerCell;

  viskores::cont::ArrayHandle<viskores::Int32> pointIdKey;
  viskores::cont::ArrayCopy(this->Connectivity, pointIdKey);

  this->RConnectivity.Allocate(rconnSize);
  invoke(ComputeReverseMapping{},
         viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, 1, this->NumberOfCellsPerPlane),
         this->RConnectivity);

  viskores::cont::Algorithm::SortByKey(pointIdKey, this->RConnectivity);

  // now we can compute the counts and offsets
  viskores::cont::ArrayHandle<viskores::Int32> reducedKeys;
  viskores::cont::Algorithm::ReduceByKey(
    pointIdKey,
    viskores::cont::make_ArrayHandleConstant(viskores::Int32(1),
                                             static_cast<viskores::Int32>(rconnSize)),
    reducedKeys,
    this->RCounts,
    viskores::Add{});

  viskores::cont::Algorithm::ScanExclusive(this->RCounts, this->ROffsets);

  // compute PrevNode from NextNode
  this->PrevNode.Allocate(this->NextNode.GetNumberOfValues());
  invoke(ComputePrevNode{}, this->NextNode, this->PrevNode);

  this->ReverseConnectivityBuilt = true;
}

viskores::exec::ConnectivityExtrude CellSetExtrude::PrepareForInput(
  viskores::cont::DeviceAdapterId device,
  viskores::TopologyElementTagCell,
  viskores::TopologyElementTagPoint,
  viskores::cont::Token& token) const
{
  return viskores::exec::ConnectivityExtrude(this->Connectivity.PrepareForInput(device, token),
                                             this->NextNode.PrepareForInput(device, token),
                                             this->NumberOfCellsPerPlane,
                                             this->NumberOfPointsPerPlane,
                                             this->NumberOfPlanes,
                                             this->IsPeriodic);
}

viskores::exec::ReverseConnectivityExtrude CellSetExtrude::PrepareForInput(
  viskores::cont::DeviceAdapterId device,
  viskores::TopologyElementTagPoint,
  viskores::TopologyElementTagCell,
  viskores::cont::Token& token) const
{
  if (!this->ReverseConnectivityBuilt)
  {
    viskores::cont::ScopedRuntimeDeviceTracker tracker(device);
    const_cast<CellSetExtrude*>(this)->BuildReverseConnectivity();
  }
  return viskores::exec::ReverseConnectivityExtrude(
    this->RConnectivity.PrepareForInput(device, token),
    this->ROffsets.PrepareForInput(device, token),
    this->RCounts.PrepareForInput(device, token),
    this->PrevNode.PrepareForInput(device, token),
    this->NumberOfCellsPerPlane,
    this->NumberOfPointsPerPlane,
    this->NumberOfPlanes);
}

}
} // viskores::cont
