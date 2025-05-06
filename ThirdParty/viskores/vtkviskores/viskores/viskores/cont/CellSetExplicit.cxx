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
#define viskores_cont_CellSetExplicit_cxx

#include <viskores/cont/CellSetExplicit.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DefaultTypes.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/internal/ReverseConnectivityBuilder.h>

namespace
{

template <typename ConnectStorage, typename OffsetStorage>
void DoBuildReverseConnectivity(
  const viskores::cont::ArrayHandle<viskores::Id, ConnectStorage>& connections,
  const viskores::cont::ArrayHandle<viskores::Id, OffsetStorage>& offsets,
  viskores::Id numberOfPoints,
  viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  viskores::cont::DeviceAdapterId suggestedDevice)
{
  using CellsWithPointsConnectivity = viskores::cont::internal::
    ConnectivityExplicitInternals<VISKORES_DEFAULT_STORAGE_TAG, ConnectStorage, OffsetStorage>;

  // Make a fake visitCellsWithPoints to pass to ComputeRConnTable. This is a bit of a
  // patchwork from changing implementation.
  CellsWithPointsConnectivity visitCellsWithPoints;
  visitCellsWithPoints.ElementsValid = true;
  visitCellsWithPoints.Connectivity = connections;
  visitCellsWithPoints.Offsets = offsets;

  bool success = viskores::cont::TryExecuteOnDevice(
    suggestedDevice,
    [&](viskores::cont::DeviceAdapterId realDevice)
    {
      viskores::cont::internal::ComputeRConnTable(
        visitPointsWithCells, visitCellsWithPoints, numberOfPoints, realDevice);
      return true;
    });

  if (!success)
  {
    throw viskores::cont::ErrorExecution("Failed to run CellSetExplicit reverse "
                                         "connectivity builder.");
  }
}

struct BuildReverseConnectivityForCellSetType
{
  template <typename ShapeStorage, typename ConnectStorage, typename OffsetStorage>
  void BuildExplicit(
    const viskores::cont::CellSetExplicit<ShapeStorage, ConnectStorage, OffsetStorage>*,
    const viskores::cont::UnknownArrayHandle* connections,
    const viskores::cont::UnknownArrayHandle* offsets,
    viskores::Id numberOfPoints,
    viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit* visitPointsWithCells,
    viskores::cont::DeviceAdapterId* device) const
  {
    if (visitPointsWithCells->ElementsValid)
    {
      return; // Already computed reverse
    }

    using ConnectArrayType = viskores::cont::ArrayHandle<viskores::Id, ConnectStorage>;
    using OffsetArrayType = viskores::cont::ArrayHandle<viskores::Id, OffsetStorage>;
    if (connections->CanConvert<ConnectArrayType>() && offsets->CanConvert<OffsetArrayType>())
    {
      DoBuildReverseConnectivity(connections->AsArrayHandle<ConnectArrayType>(),
                                 offsets->AsArrayHandle<OffsetArrayType>(),
                                 numberOfPoints,
                                 *visitPointsWithCells,
                                 *device);
    }
  }

  void BuildExplicit(...) const
  {
    // Not an explicit cell set, so skip.
  }

  // Note that we are doing something a little weird here. We are taking a method templated
  // on the cell set type, getting pointers of many of the arguments, and then calling
  // a different overloaded method to do the actual work. Here is why.
  //
  // Our ultimate goal is to call one method if operating on `CellSetExplict` or
  // _any subclass_. We also want to ignore any cell type that is not a `CellSetExplicit`
  // or one of its sublcasses. So we want one templated method that specializes on a
  // `CellSetExplicit` with its three template arguments and another that is templated on
  // any `CellSet`. That works for a class of `CellSetExplicit`, but not of a subclass.
  // A subclass will match the more generic class.
  //
  // We can get around that by use variadic arguments (e.g. `...` for the arguments), which
  // we can easily do since we just want to ignore the arguments. C++ will match the arguments
  // with templates before matching the C variadic arguments. However, these variadic
  // arguments only work for POD types. To convert to POD types, we make a new overloaded
  // method that accepts pointers instead. (Not sure why pointers work but references do not.)
  template <typename CellSetType>
  void operator()(
    const CellSetType& cellset,
    const viskores::cont::UnknownArrayHandle& connections,
    const viskores::cont::UnknownArrayHandle& offsets,
    viskores::Id numberOfPoints,
    viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
    viskores::cont::DeviceAdapterId device) const
  {
    this->BuildExplicit(
      &cellset, &connections, &offsets, numberOfPoints, &visitPointsWithCells, &device);
  }
};

} // anonymous namespace

namespace viskores
{
namespace cont
{

template class VISKORES_CONT_EXPORT CellSetExplicit<>; // default
template class VISKORES_CONT_EXPORT
  CellSetExplicit<typename viskores::cont::ArrayHandleConstant<viskores::UInt8>::StorageTag,
                  VISKORES_DEFAULT_CONNECTIVITY_STORAGE_TAG,
                  typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag>;

namespace detail
{

void BuildReverseConnectivity(
  const viskores::cont::UnknownArrayHandle& connections,
  const viskores::cont::UnknownArrayHandle& offsets,
  viskores::Id numberOfPoints,
  viskores::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  viskores::cont::DeviceAdapterId device)
{
  if (visitPointsWithCells.ElementsValid)
  {
    return; // Already computed
  }

  viskores::ListForEach(BuildReverseConnectivityForCellSetType{},
                        VISKORES_DEFAULT_CELL_SET_LIST{},
                        connections,
                        offsets,
                        numberOfPoints,
                        visitPointsWithCells,
                        device);

  if (!visitPointsWithCells.ElementsValid)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "BuildReverseConnectivity attempted for all known cell set types; "
                   "falling back to copy connectivity arrays.");
    viskores::cont::ArrayHandle<viskores::Id> connectionsCopy;
    viskores::cont::ArrayCopy(connections, connectionsCopy);
    viskores::cont::ArrayHandle<viskores::Id> offsetsCopy;
    viskores::cont::ArrayCopy(offsets, offsetsCopy);
    DoBuildReverseConnectivity(
      connectionsCopy, offsetsCopy, numberOfPoints, visitPointsWithCells, device);
  }
}

} // namespace viskores::cont::detail

} // namespace viskores::cont
} // namespace viskores
