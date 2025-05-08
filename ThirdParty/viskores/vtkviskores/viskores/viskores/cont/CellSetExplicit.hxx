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
#ifndef viskores_cont_CellSetExplicit_hxx
#define viskores_cont_CellSetExplicit_hxx
#include <viskores/cont/CellSetExplicit.h>

#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/Logging.h>

#include <viskores/Deprecated.h>

// This file uses a lot of very verbose identifiers and the clang formatted
// code quickly becomes unreadable. Stick with manual formatting for now.
//
// clang-format off

namespace viskores
{
namespace cont
{

template <typename SST, typename CST, typename OST>
VISKORES_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit()
  : CellSet()
  , Data(std::make_shared<Internals>())
{
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(const Thisclass& src)
  : CellSet(src)
  , Data(src.Data)
{
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(Thisclass &&src) noexcept
  : CellSet(std::forward<CellSet>(src))
  , Data(std::move(src.Data))
{
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(const Thisclass& src)
-> Thisclass&
{
  this->CellSet::operator=(src);
  this->Data = src.Data;
  return *this;
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(Thisclass&& src) noexcept
-> Thisclass&
{
  this->CellSet::operator=(std::forward<CellSet>(src));
  this->Data = std::move(src.Data);
  return *this;
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
CellSetExplicit<SST, CST, OST>::~CellSetExplicit()
{
  // explicitly define instead of '=default' to workaround an intel compiler bug
  // (see #179)
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::PrintSummary(std::ostream& out) const
{
  out << "   ExplicitCellSet:" << std::endl;
  out << "   CellPointIds:" << std::endl;
  this->Data->CellPointIds.PrintSummary(out);
  out << "   PointCellIds:" << std::endl;
  this->Data->PointCellIds.PrintSummary(out);
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::ReleaseResourcesExecution()
{
  this->Data->CellPointIds.ReleaseResourcesExecution();
  this->Data->PointCellIds.ReleaseResourcesExecution();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>::GetNumberOfCells() const
{
  return this->Data->CellPointIds.GetNumberOfElements();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>::GetNumberOfPoints() const
{
  return this->Data->NumberOfPoints;
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>::GetNumberOfFaces() const
{
  return -1;
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>::GetNumberOfEdges() const
{
  return -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::GetCellPointIds(viskores::Id cellId,
                                                     viskores::Id* ptids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const viskores::Id start = offPortal.Get(cellId);
  const viskores::Id end = offPortal.Get(cellId + 1);
  const viskores::IdComponent numIndices = static_cast<viskores::IdComponent>(end - start);
  auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();
  for (viskores::IdComponent i = 0; i < numIndices; i++)
  {
    ptids[i] = connPortal.Get(start + i);
  }
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(viskores::TopologyElementTagCell) const
{
  return this->GetNumberOfCells();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(viskores::TopologyElementTagPoint) const
{
  return this->GetNumberOfPoints();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
viskores::IdComponent CellSetExplicit<SST, CST, OST>
::GetNumberOfPointsInCell(viskores::Id cellid) const
{
  const auto portal = this->Data->CellPointIds.Offsets.ReadPortal();
  return static_cast<viskores::IdComponent>(portal.Get(cellid + 1) -
                                        portal.Get(cellid));
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
typename viskores::cont::ArrayHandle<viskores::UInt8, SST>::ReadPortalType
CellSetExplicit<SST, CST, OST>::ShapesReadPortal() const
{
  return this->Data->CellPointIds.Shapes.ReadPortal();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
VISKORES_DEPRECATED(1.6, "Calling GetCellShape(cellid) is a performance bug. Call ShapesReadPortal() and loop over the Get.")
viskores::UInt8 CellSetExplicit<SST, CST, OST>
::GetCellShape(viskores::Id cellid) const
{
  return this->ShapesReadPortal().Get(cellid);
}


template <typename SST, typename CST, typename OST>
template <viskores::IdComponent NumVecIndices>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(viskores::Id cellId, viskores::Vec<viskores::Id, NumVecIndices>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const viskores::Id start = offPortal.Get(cellId);
  const viskores::Id end = offPortal.Get(cellId + 1);
  const auto numCellIndices = static_cast<viskores::IdComponent>(end - start);
  const auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();

  VISKORES_LOG_IF_S(viskores::cont::LogLevel::Warn,
                numCellIndices != NumVecIndices,
                "GetIndices given a " << NumVecIndices
                << "-vec to fetch a cell with " << numCellIndices << "points. "
                "Truncating result.");

  const viskores::IdComponent numIndices = viskores::Min(NumVecIndices, numCellIndices);

  for (viskores::IdComponent i = 0; i < numIndices; i++)
  {
    ids[i] = connPortal.Get(start + i);
  }
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(viskores::Id cellId, viskores::cont::ArrayHandle<viskores::Id>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.ReadPortal();
  const viskores::Id start = offPortal.Get(cellId);
  const viskores::Id end = offPortal.Get(cellId + 1);
  const viskores::IdComponent numIndices = static_cast<viskores::IdComponent>(end - start);
  ids.Allocate(numIndices);
  auto connPortal = this->Data->CellPointIds.Connectivity.ReadPortal();

  auto outIdPortal = ids.WritePortal();

  for (viskores::IdComponent i = 0; i < numIndices; i++)
  {
    outIdPortal.Set(i, connPortal.Get(start + i));
  }
}


//----------------------------------------------------------------------------
namespace internal
{

// Sets the first value of the array to zero if the handle is writable,
// otherwise do nothing:
template <typename ArrayType>
typename std::enable_if<viskores::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&& array)
{
  using ValueType = typename std::decay<ArrayType>::type::ValueType;
  using Traits = viskores::TypeTraits<ValueType>;
  array.WritePortal().Set(0, Traits::ZeroInitialization());
}

template <typename ArrayType>
typename std::enable_if<!viskores::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&&)
{ /* no-op */ }

} // end namespace internal

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>
::PrepareToAddCells(viskores::Id numCells,
                    viskores::Id connectivityMaxLen)
{
  this->Data->CellPointIds.Shapes.Allocate(numCells);
  this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);
  this->Data->CellPointIds.Offsets.Allocate(numCells + 1);
  internal::SetFirstToZeroIfWritable(this->Data->CellPointIds.Offsets);
  this->Data->NumberOfCellsAdded = 0;
  this->Data->ConnectivityAdded = 0;
}

template <typename SST, typename CST, typename OST>
template <typename IdVecType>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::AddCell(viskores::UInt8 cellType,
                                             viskores::IdComponent numVertices,
                                             const IdVecType& ids)
{
  using Traits = viskores::VecTraits<IdVecType>;
  VISKORES_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, viskores::Id>::value),
                         "CellSetSingleType::AddCell requires viskores::Id for indices.");

  if (Traits::GetNumberOfComponents(ids) < numVertices)
  {
    throw viskores::cont::ErrorBadValue("Not enough indices given to CellSetExplicit::AddCell.");
  }

  if (this->Data->NumberOfCellsAdded >= this->Data->CellPointIds.Shapes.GetNumberOfValues())
  {
    throw viskores::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->Data->ConnectivityAdded + numVertices >
      this->Data->CellPointIds.Connectivity.GetNumberOfValues())
  {
    throw viskores::cont::ErrorBadValue(
      "Connectivity increased past estimated maximum connectivity.");
  }

  auto shapes = this->Data->CellPointIds.Shapes.WritePortal();
  auto conn = this->Data->CellPointIds.Connectivity.WritePortal();
  auto offsets = this->Data->CellPointIds.Offsets.WritePortal();

  shapes.Set(this->Data->NumberOfCellsAdded, cellType);
  for (viskores::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    conn.Set(this->Data->ConnectivityAdded + iVec,
             Traits::GetComponent(ids, iVec));
  }

  this->Data->NumberOfCellsAdded++;
  this->Data->ConnectivityAdded += numVertices;

  // Set the end offset for the added cell:
  offsets.Set(this->Data->NumberOfCellsAdded, this->Data->ConnectivityAdded);
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::CompleteAddingCells(viskores::Id numPoints)
{
  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Connectivity.Allocate(this->Data->ConnectivityAdded, viskores::CopyFlag::On);
  this->Data->CellPointIds.ElementsValid = true;

  if (this->Data->NumberOfCellsAdded != this->GetNumberOfCells())
  {
    throw viskores::cont::ErrorBadValue("Did not add as many cells as expected.");
  }

  this->Data->NumberOfCellsAdded = -1;
  this->Data->ConnectivityAdded = -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>
::Fill(viskores::Id numPoints,
       const viskores::cont::ArrayHandle<viskores::UInt8, SST>& shapes,
       const viskores::cont::ArrayHandle<viskores::Id, CST>& connectivity,
       const viskores::cont::ArrayHandle<viskores::Id, OST>& offsets)
{
  // Validate inputs:
  // Even for an empty cellset, offsets must contain a single 0:
  VISKORES_ASSERT(offsets.GetNumberOfValues() > 0);
  // Must be [numCells + 1] offsets and [numCells] shapes
  VISKORES_ASSERT(offsets.GetNumberOfValues() == shapes.GetNumberOfValues() + 1);
  // The last offset must be the size of the connectivity array.
  VISKORES_ASSERT(viskores::cont::ArrayGetValue(offsets.GetNumberOfValues() - 1,
                                        offsets) ==
              connectivity.GetNumberOfValues());

  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Shapes = shapes;
  this->Data->CellPointIds.Connectivity = connectivity;
  this->Data->CellPointIds.Offsets = offsets;

  this->Data->CellPointIds.ElementsValid = true;

  this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>
::PrepareForInput(viskores::cont::DeviceAdapterId device, VisitTopology, IncidentTopology, viskores::cont::Token& token) const
-> ExecConnectivityType<VisitTopology, IncidentTopology>
{
  this->BuildConnectivity(device, VisitTopology{}, IncidentTopology{});

  const auto& connectivity = this->GetConnectivity(VisitTopology{},
                                                   IncidentTopology{});
  VISKORES_ASSERT(connectivity.ElementsValid);

  using ExecObjType = ExecConnectivityType<VisitTopology, IncidentTopology>;

  return ExecObjType(connectivity.Shapes.PrepareForInput(device, token),
                     connectivity.Connectivity.PrepareForInput(device, token),
                     connectivity.Offsets.PrepareForInput(device, token));
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VISKORES_CONT auto CellSetExplicit<SST, CST, OST>
::GetShapesArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ShapesArrayType&
{
  this->BuildConnectivity(viskores::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{}, IncidentTopology{}).Shapes;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>
::GetConnectivityArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ConnectivityArrayType&
{
  this->BuildConnectivity(viskores::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Connectivity;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>
::GetOffsetsArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::OffsetsArrayType&
{
  this->BuildConnectivity(viskores::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Offsets;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
VISKORES_CONT
auto CellSetExplicit<SST, CST, OST>
::GetNumIndicesArray(VisitTopology visited, IncidentTopology incident) const
-> typename ConnectivityChooser<VisitTopology,
                                IncidentTopology>::NumIndicesArrayType
{
  // Converts to NumIndicesArrayType (which is an ArrayHandleOffsetsToNumComponents)
  return this->GetOffsetsArray(visited, incident);
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
VISKORES_CONT
std::shared_ptr<CellSet> CellSetExplicit<SST, CST, OST>::NewInstance() const
{
  return std::make_shared<CellSetExplicit>();
}

template <typename SST, typename CST, typename OST>
VISKORES_CONT
void CellSetExplicit<SST, CST, OST>::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExplicit*>(src);
  if (!other)
  {
    throw viskores::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  ShapesArrayType shapes;
  ConnectivityArrayType conn;
  OffsetsArrayType offsets;

  const auto ct = viskores::TopologyElementTagCell{};
  const auto pt = viskores::TopologyElementTagPoint{};

  shapes.DeepCopyFrom(other->GetShapesArray(ct, pt));
  conn.DeepCopyFrom(other->GetConnectivityArray(ct, pt));
  offsets.DeepCopyFrom(other->GetOffsetsArray(ct, pt));

  this->Fill(other->GetNumberOfPoints(), shapes, conn, offsets);
}

}
} // viskores::cont

// clang-format on

#endif
