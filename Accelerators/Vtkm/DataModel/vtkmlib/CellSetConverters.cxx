// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "CellSetConverters.h"

#include "ArrayConverters.hxx"
#include "DataSetConverters.h"

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/BinaryPredicates.h>
#include <viskores/Swap.h>

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmDataArray.h"

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN
vtkIdType IsHomogeneous(vtkCellArray* cells)
{
  auto offsets = cells->GetOffsetsArray();
  using ArrayHandleCounting64 = viskores::cont::ArrayHandleCounting<viskores::Int64>;
  using ArrayHandleCounting32 = viskores::cont::ArrayHandleCounting<viskores::Int32>;
  using ArrayHandleCounting64Cast =
    viskores::cont::ArrayHandleCast<viskores::Int64, ArrayHandleCounting32>;
  using ArrayHandleCounting32Cast =
    viskores::cont::ArrayHandleCast<viskores::Int32, ArrayHandleCounting64>;
  if (auto offsets64 = vtkmDataArray<viskores::Int64>::SafeDownCast(offsets))
  {
    auto unknownArrayHandle = offsets64->GetVtkmUnknownArrayHandle();
    if (unknownArrayHandle.CanConvert<ArrayHandleCounting64>())
    {
      auto arrayHandleCounting = unknownArrayHandle.AsArrayHandle<ArrayHandleCounting64>();
      return arrayHandleCounting.GetStep();
    }
    if (unknownArrayHandle.CanConvert<ArrayHandleCounting64Cast>())
    {
      auto arrayHandleCounting = unknownArrayHandle.AsArrayHandle<ArrayHandleCounting64Cast>();
      return arrayHandleCounting.GetSourceArray().GetStep();
    }
    return -1; // not homogeneous
  }
  else if (auto offsets32 = vtkmDataArray<viskores::Int32>::SafeDownCast(offsets))
  {
    auto unknownArrayHandle = offsets32->GetVtkmUnknownArrayHandle();
    if (unknownArrayHandle.CanConvert<ArrayHandleCounting32>())
    {
      auto arrayHandleCounting = unknownArrayHandle.AsArrayHandle<ArrayHandleCounting32>();
      return arrayHandleCounting.GetStep();
    }
    if (unknownArrayHandle.CanConvert<ArrayHandleCounting32Cast>())
    {
      auto arrayHandleCounting = unknownArrayHandle.AsArrayHandle<ArrayHandleCounting32Cast>();
      return arrayHandleCounting.GetSourceArray().GetStep();
    }
    return -1; // not homogeneous
  }
  else
  {
    return cells->IsHomogeneous();
  }
}

bool IsHomogeneous(vtkUnstructuredGrid* ugrid)
{
  return ugrid->IsHomogeneous();
}

namespace
{

template <int Size, typename Enable = void>
struct ReorderQuadHex;

// Specialization for 4 points
template <int Size>
struct ReorderQuadHex<Size, std::enable_if_t<Size == 4>> : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  VISKORES_EXEC void operator()(viskores::Vec<viskores::Id, Size>& indices) const
  {
    viskores::Swap(indices[2], indices[3]);
  }
};

// Specialization for 8 points
template <int Size>
struct ReorderQuadHex<Size, std::enable_if_t<Size == 8>> : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  VISKORES_EXEC void operator()(viskores::Vec<viskores::Id, Size>& indices) const
  {
    viskores::Swap(indices[2], indices[3]);
    viskores::Swap(indices[6], indices[7]);
  }
};

bool CanRunOnGPU()
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  return tracker.CanRunOn(viskores::cont::DeviceAdapterTagCuda{}) ||
    tracker.CanRunOn(viskores::cont::DeviceAdapterTagKokkos{});
}

struct BuildSingleTypeCellSetVisitor
{
#define SINGLE_TYPE_CELLSET_FROM_VTKM_DATA_ARRAY(dataType)                                         \
  if (auto connAsVtkmArray = vtkmDataArray<dataType>::SafeDownCast(connectivity))                  \
  {                                                                                                \
    auto connHandleUnknown = connAsVtkmArray->GetVtkmUnknownArrayHandle();                         \
    using ArrayHandle = viskores::cont::ArrayHandleBasic<dataType>;                                \
    if (!connHandleUnknown.template CanConvert<ArrayHandle>())                                     \
    {                                                                                              \
      throw viskores::cont::ErrorBadType("Unsupported VTK connectivity array type in "             \
                                         "CellSetSingleType converter.");                          \
    }                                                                                              \
    auto connHandleDirect = connHandleUnknown.template AsArrayHandle<ArrayHandle>();               \
    constexpr bool IsViskoresIdType = std::is_same_v<dataType, viskores::Id>;                      \
    auto connHandle = IsViskoresIdType                                                             \
      ? connHandleDirect                                                                           \
      : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);                      \
    using ConnStorageTag = typename std::decay_t<decltype(connHandle)>::StorageTag;                \
    viskores::cont::CellSetSingleType<ConnStorageTag> cellSet;                                     \
    cellSet.Fill(static_cast<viskores::Id>(numPoints), cellType, cellSize, connHandle);            \
    return cellSet;                                                                                \
  }
#define SINGLE_TYPE_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(arrayCls, dataType)                      \
  if (auto connAsAOSArray = arrayCls<dataType>::SafeDownCast(connectivity))                        \
  {                                                                                                \
    auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(connAsAOSArray);              \
    constexpr bool IsViskoresIdType = std::is_same_v<dataType, viskores::Id>;                      \
    auto connHandle = IsViskoresIdType                                                             \
      ? connHandleDirect                                                                           \
      : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);                      \
    using ConnStorageTag = typename std::decay_t<decltype(connHandle)>::StorageTag;                \
    viskores::cont::CellSetSingleType<ConnStorageTag> cellSet;                                     \
    cellSet.Fill(static_cast<viskores::Id>(numPoints), cellType, cellSize, connHandle);            \
    return cellSet;                                                                                \
  }
#define SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(arrayCls, dataType)                    \
  if (auto connAsAOSArray = arrayCls<dataType>::SafeDownCast(connectivity))                        \
  {                                                                                                \
    auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(connAsAOSArray);              \
    if (CanRunOnGPU() || forceViskores)                                                            \
    {                                                                                              \
      viskores::cont::ArrayHandleBasic<viskores::Id> connHandle;                                   \
      viskores::cont::ArrayCopyDevice(connHandleDirect, connHandle);                               \
      viskores::cont::CellSetSingleType<> cellSet;                                                 \
      cellSet.Fill(static_cast<viskores::Id>(numPoints), cellType, cellSize, connHandle);          \
      return cellSet;                                                                              \
    }                                                                                              \
  }

  viskores::cont::UnknownCellSet operator()(vtkCellArray* cells, viskores::UInt8 cellType,
    viskores::IdComponent cellSize, vtkIdType numPoints, bool forceViskores)
  {
    auto connectivity = cells->GetConnectivityArray();
    SINGLE_TYPE_CELLSET_FROM_VTKM_DATA_ARRAY(viskores::Int64);
    SINGLE_TYPE_CELLSET_FROM_VTKM_DATA_ARRAY(viskores::Int32);
    SINGLE_TYPE_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int64);
    SINGLE_TYPE_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int32);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int16);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int8);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt64);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt32);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt16);
    SINGLE_TYPE_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt8);
    if (forceViskores)
    {
      // Fallback if none of the above worked.
      // Construct an arrayhandle that holds the connectivity array
      auto connRange = vtk::DataArrayValueRange<1, vtkIdType>(connectivity);
      viskores::cont::ArrayHandleBasic<viskores::Id> connHandleDirect;
      connHandleDirect.Allocate(connRange.size());
      std::copy(connRange.begin(), connRange.end(),
        viskores::cont::ArrayPortalToIteratorBegin(connHandleDirect.WritePortal()));
      viskores::cont::CellSetSingleType<> cellSet;
      cellSet.Fill(static_cast<viskores::Id>(numPoints), cellType, cellSize, connHandleDirect);
      return cellSet;
    }
    throw viskores::cont::ErrorBadType("Unsupported VTK connectivity array type in "
                                       "CellSetSingleType converter.");
  }
};

template <int Size>
struct BuildSingleTypePixelVoxelCellSetVisitor
{
#define SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(arrayCls, dataType)                   \
  if (auto connAsAOSArray = vtkAOSDataArrayTemplate<dataType>::SafeDownCast(connectivity))         \
  {                                                                                                \
    auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(connAsAOSArray);              \
    if (CanRunOnGPU() || forceViskores)                                                            \
    {                                                                                              \
      viskores::cont::ArrayHandleBasic<viskores::Id> connHandle;                                   \
      viskores::cont::ArrayCopyDevice(connHandleDirect, connHandle);                               \
      viskores::cont::Invoker invoke;                                                              \
      invoke(TReorderQuadHex{}, viskores::cont::make_ArrayHandleGroupVec<Size>(connHandle));       \
      viskores::cont::CellSetSingleType<> cellSet;                                                 \
      cellSet.Fill(numPoints, cellType, Size, connHandle);                                         \
      return cellSet;                                                                              \
    }                                                                                              \
  }

  viskores::cont::UnknownCellSet operator()(
    vtkCellArray* cells, viskores::UInt8 cellType, vtkIdType numPoints, bool forceViskores)
  {
    using TReorderQuadHex = ReorderQuadHex<Size>;
    auto connectivity = cells->GetConnectivityArray();
    if (vtkmDataArray<viskores::Id>::SafeDownCast(connectivity))
    {
      throw viskores::cont::ErrorBadType("Viskores does not export voxel cell types.");
    }
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int64);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int32);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int16);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int8);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt64);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt32);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt16);
    SINGLE_TYPE_CELLSET_PIVO_XEL_FROM_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt8);
    if (!forceViskores)
    {
      throw viskores::cont::ErrorBadType("Unsupported VTK connectivity array type in "
                                         "CellSetSingleType converter.");
    }
    viskores::cont::ArrayHandleBasic<viskores::Id> connHandle;
    auto connRange = vtk::DataArrayValueRange<1, vtkIdType>(connectivity);
    connHandle.Allocate(connRange.size());
    std::copy(connRange.begin(), connRange.end(),
      viskores::cont::ArrayPortalToIteratorBegin(connHandle.WritePortal()));

    // reorder cells from voxel->hex
    viskores::cont::Invoker invoke;
    invoke(TReorderQuadHex{}, viskores::cont::make_ArrayHandleGroupVec<Size>(connHandle));

    viskores::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(numPoints, viskores::CELL_SHAPE_HEXAHEDRON, Size, connHandle);
    return cellSet;
  }
};
} // end anon namespace

// convert a cell array of a single type to a viskores CellSetSingleType
viskores::cont::UnknownCellSet ConvertSingleType(
  vtkCellArray* cells, int cellType, vtkIdType numberOfPoints, bool forceViskores)
{
  switch (cellType)
  {
    case VTK_LINE:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_LINE, 2, numberOfPoints, forceViskores);

    case VTK_HEXAHEDRON:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_HEXAHEDRON, 8, numberOfPoints, forceViskores);

    case VTK_VOXEL:
      // Note that this is a special case that reorders ids voxel to hex:
      return BuildSingleTypePixelVoxelCellSetVisitor<8>{}(
        cells, viskores::CELL_SHAPE_HEXAHEDRON, numberOfPoints, forceViskores);

    case VTK_QUAD:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_QUAD, 4, numberOfPoints, forceViskores);

    case VTK_PIXEL:
      // Note that this is a special case that reorders ids pixel to quad:
      return BuildSingleTypePixelVoxelCellSetVisitor<4>{}(
        cells, viskores::CELL_SHAPE_QUAD, numberOfPoints, forceViskores);

    case VTK_TETRA:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_TETRA, 4, numberOfPoints, forceViskores);

    case VTK_TRIANGLE:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_TRIANGLE, 3, numberOfPoints, forceViskores);

    case VTK_VERTEX:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_VERTEX, 1, numberOfPoints, forceViskores);

    case VTK_WEDGE:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_WEDGE, 6, numberOfPoints, forceViskores);

    case VTK_PYRAMID:
      return BuildSingleTypeCellSetVisitor{}(
        cells, viskores::CELL_SHAPE_PYRAMID, 5, numberOfPoints, forceViskores);

    default:
      throw viskores::cont::ErrorBadType("Unsupported VTK cell type in "
                                         "CellSetSingleType converter.");
  }
}

namespace
{

struct BuildExplicitCellSetVisitor
{
#define EXPICIT_CELLSET_FROM_VTKM_DATA_ARRAY(dataType)                                             \
  if (auto connAsVtkmArray = vtkmDataArray<dataType>::SafeDownCast(connectivity))                  \
  {                                                                                                \
    if (auto offsetsAsVtkmArray = vtkmDataArray<dataType>::SafeDownCast(offsets))                  \
    {                                                                                              \
      auto connHandleUnknown = connAsVtkmArray->GetVtkmUnknownArrayHandle();                       \
      auto offsetsHandleUnknown = offsetsAsVtkmArray->GetVtkmUnknownArrayHandle();                 \
      using ArrayHandle = viskores::cont::ArrayHandleBasic<dataType>;                              \
      if (!connHandleUnknown.template CanConvert<ArrayHandle>())                                   \
      {                                                                                            \
        throw viskores::cont::ErrorBadType("Unsupported VTK connectivity array type in "           \
                                           "CellSetSingleType converter.");                        \
      }                                                                                            \
      if (!offsetsHandleUnknown.template CanConvert<ArrayHandle>())                                \
      {                                                                                            \
        throw viskores::cont::ErrorBadType("Unsupported VTK offsets array type in "                \
                                           "CellSetSingleType converter.");                        \
      }                                                                                            \
      auto connHandleDirect = connHandleUnknown.template AsArrayHandle<ArrayHandle>();             \
      auto offsetsHandleDirect = offsetsHandleUnknown.template AsArrayHandle<ArrayHandle>();       \
      constexpr bool IsViskoresIdType = std::is_same_v<dataType, viskores::Id>;                    \
      auto connHandle = IsViskoresIdType                                                           \
        ? connHandleDirect                                                                         \
        : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);                    \
      auto offsetsHandle = IsViskoresIdType                                                        \
        ? offsetsHandleDirect                                                                      \
        : viskores::cont::make_ArrayHandleCast<viskores::Id>(offsetsHandleDirect);                 \
      using ShapesStorageTag = typename std::decay_t<decltype(shapes)>::StorageTag;                \
      using ConnStorageTag = typename std::decay_t<decltype(connHandle)>::StorageTag;              \
      using OffsetsStorageTag = typename decltype(offsetsHandle)::StorageTag;                      \
      viskores::cont::CellSetExplicit<ShapesStorageTag, ConnStorageTag, OffsetsStorageTag>         \
        cellSet;                                                                                   \
      cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);                                  \
      return cellSet;                                                                              \
    }                                                                                              \
  }
#define EXPLICIT_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(arrayCls, dataType)                         \
  if (auto connAsAosArray = arrayCls<dataType>::SafeDownCast(connectivity))                        \
  {                                                                                                \
    if (auto offsetsAsAosArray = arrayCls<dataType>::SafeDownCast(offsets))                        \
    {                                                                                              \
      auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(connAsAosArray);            \
      auto offsetsHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(offsetsAsAosArray);      \
      constexpr bool IsViskoresIdType = std::is_same_v<dataType, viskores::Id>;                    \
      auto connHandle = IsViskoresIdType                                                           \
        ? connHandleDirect                                                                         \
        : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);                    \
      auto offsetsHandle = IsViskoresIdType                                                        \
        ? offsetsHandleDirect                                                                      \
        : viskores::cont::make_ArrayHandleCast<viskores::Id>(offsetsHandleDirect);                 \
      using ShapesStorageTag = typename std::decay_t<decltype(shapes)>::StorageTag;                \
      using ConnStorageTag = typename std::decay_t<decltype(connHandle)>::StorageTag;              \
      using OffsetsStorageTag = typename decltype(offsetsHandle)::StorageTag;                      \
      viskores::cont::CellSetExplicit<ShapesStorageTag, ConnStorageTag, OffsetsStorageTag>         \
        cellSet;                                                                                   \
      cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);                                  \
      return cellSet;                                                                              \
    }                                                                                              \
  }

#define EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(arrayCls, dataType)                       \
  if (auto connAsAosArray = arrayCls<dataType>::SafeDownCast(connectivity))                        \
  {                                                                                                \
    if (auto offsetsAsAosArray = arrayCls<dataType>::SafeDownCast(offsets))                        \
    {                                                                                              \
      auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(connAsAosArray);            \
      auto offsetsHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(offsetsAsAosArray);      \
      if (CanRunOnGPU() || forceViskores)                                                          \
      {                                                                                            \
        viskores::cont::ArrayHandleBasic<viskores::Id> offsetsHandle, connHandle;                  \
        viskores::cont::ArrayCopyDevice(offsetsHandleDirect, offsetsHandle);                       \
        viskores::cont::ArrayCopyDevice(connHandleDirect, connHandle);                             \
        viskores::cont::CellSetExplicit<> cellSet;                                                 \
        cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);                                \
        return cellSet;                                                                            \
      }                                                                                            \
    }                                                                                              \
  }

  viskores::cont::UnknownCellSet operator()(vtkCellArray* cells,
    const viskores::cont::ArrayHandleBasic<viskores::UInt8>& shapes, viskores::Id numPoints,
    bool forceViskores) const
  {
    auto connectivity = cells->GetConnectivityArray();
    auto offsets = cells->GetOffsetsArray();
    EXPICIT_CELLSET_FROM_VTKM_DATA_ARRAY(viskores::Int64);
    EXPICIT_CELLSET_FROM_VTKM_DATA_ARRAY(viskores::Int32);
    EXPLICIT_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int64);
    EXPLICIT_CELLSET_FROM_KNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int32);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int16);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::Int8);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt64);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt32);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt16);
    EXPLICIT_CELLSET_FROM_UNKNOWN_VTK_AOS_DATA_ARRAY(vtkAOSDataArrayTemplate, viskores::UInt8);
    if (!forceViskores)
    {
      throw viskores::cont::ErrorBadType("Unsupported VTK connectivity or offsets array type in "
                                         "CellSetExplicit converter.");
    }
    // Fallback to vtkDataArray
    // Construct arrayhandles to hold the arrays
    auto offsetsRange = vtk::DataArrayValueRange<1, vtkIdType>(offsets);
    auto connRange = vtk::DataArrayValueRange<1, vtkIdType>(connectivity);
    viskores::cont::ArrayHandleBasic<viskores::Id> offsetsHandleDirect, connHandleDirect;
    offsetsHandleDirect.Allocate(offsetsRange.size());
    std::copy(offsetsRange.begin(), offsetsRange.end(),
      viskores::cont::ArrayPortalToIteratorBegin(offsetsHandleDirect.WritePortal()));
    connHandleDirect.Allocate(connRange.size());
    std::copy(connRange.begin(), connRange.end(),
      viskores::cont::ArrayPortalToIteratorBegin(connHandleDirect.WritePortal()));

    viskores::cont::CellSetExplicit<> cellSet;
    cellSet.Fill(numPoints, shapes, connHandleDirect, offsetsHandleDirect);
    return cellSet;
  }
};

struct SupportedCellShape
{
  VISKORES_EXEC_CONT
  bool operator()(viskores::UInt8 shape) const
  {
    return (shape < viskores::NUMBER_OF_CELL_SHAPES) && (shape != 2) && (shape != 6) &&
      (shape != 8) && (shape != 11);
  }
};

} // end anon namespace

// convert a cell array of mixed types to a viskores CellSetExplicit
viskores::cont::UnknownCellSet Convert(
  vtkUnsignedCharArray* types, vtkCellArray* cells, vtkIdType numberOfPoints, bool forceViskores)
{
  auto shapes = tovtkm::vtkAOSDataArrayToFlatArrayHandle(types);
  if (!viskores::cont::Algorithm::Reduce(
        viskores::cont::make_ArrayHandleTransform(shapes, SupportedCellShape{}), true,
        viskores::LogicalAnd()))
  {
    throw viskores::cont::ErrorBadType("Unsupported VTK cell type in CellSet converter.");
  }
  return BuildExplicitCellSetVisitor{}(cells, shapes, numberOfPoints, forceViskores);
}

VTK_ABI_NAMESPACE_END
} // namespace tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

bool Convert(const viskores::cont::UnknownCellSet& toConvert, vtkCellArray* cells,
  vtkUnsignedCharArray* typesArray, bool forceViskores)
{
  const auto* cellset = toConvert.GetCellSetBase();
  const viskores::Id numCells = cellset->GetNumberOfCells();

  if (toConvert.CanConvert<toviskores::CellSetSingleType32Bit>())
  {
    toviskores::CellSetSingleType32Bit single;
    toConvert.AsCellSet(single);
    auto offsets = vtk::TakeSmartPointer(make_vtkmDataArray(single.GetOffsetsArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    auto connectivity = vtk::TakeSmartPointer(make_vtkmDataArray(single.GetConnectivityArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    cells->SetData(offsets, connectivity);
    if (typesArray != nullptr)
    {
      typesArray->SetNumberOfComponents(1);
      typesArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells));
      for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
      {
        const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
        typesArray->SetValue(vtkCellId, cellset->GetCellShape(cellId));
      }
    }
    return true;
  }
  if (toConvert.CanConvert<toviskores::CellSetSingleType64Bit>())
  {
    toviskores::CellSetSingleType64Bit single;
    toConvert.AsCellSet(single);
    auto offsets = vtk::TakeSmartPointer(make_vtkmDataArray(single.GetOffsetsArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    auto connectivity = vtk::TakeSmartPointer(make_vtkmDataArray(single.GetConnectivityArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    cells->SetData(offsets, connectivity);
    if (typesArray != nullptr)
    {
      typesArray->SetNumberOfComponents(1);
      typesArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells));
      for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
      {
        const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
        typesArray->SetValue(vtkCellId, cellset->GetCellShape(cellId));
      }
    }
    return true;
  }
  if (toConvert.CanConvert<toviskores::CellSetExplicit32Bit>())
  {
    toviskores::CellSetExplicit32Bit explicitCS;
    toConvert.AsCellSet(explicitCS);
    auto connectivity = vtk::TakeSmartPointer(make_vtkmDataArray(explicitCS.GetConnectivityArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    auto offsets = vtk::TakeSmartPointer(make_vtkmDataArray(explicitCS.GetOffsetsArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    cells->SetData(offsets, connectivity);
    auto shapesArray = explicitCS.GetShapesArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    if (typesArray != nullptr)
    {
      typesArray->SetNumberOfComponents(1);
      typesArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells));
      for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
      {
        const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
        typesArray->SetValue(vtkCellId, cellset->GetCellShape(cellId));
      }
    }
    return true;
  }
  if (toConvert.CanConvert<toviskores::CellSetExplicit64Bit>())
  {
    toviskores::CellSetExplicit64Bit explicitCS;
    toConvert.AsCellSet(explicitCS);
    auto connectivity = vtk::TakeSmartPointer(make_vtkmDataArray(explicitCS.GetConnectivityArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    auto offsets = vtk::TakeSmartPointer(make_vtkmDataArray(explicitCS.GetOffsetsArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint())));
    cells->SetData(offsets, connectivity);
    auto shapesArray = explicitCS.GetShapesArray(
      viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
    if (typesArray != nullptr)
    {
      typesArray->SetNumberOfComponents(1);
      typesArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells));
      for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
      {
        const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
        typesArray->SetValue(vtkCellId, cellset->GetCellShape(cellId));
      }
    }
    return true;
  }
  if (forceViskores)
  {
    throw viskores::cont::ErrorBadType("Unsupported Viskores cell set type in fromvtkm converter.");
  }

  const viskores::Id maxSize = numCells * 8; // largest cell type is hex
  // no-copy optimization.
  vtkNew<vtkIdTypeArray> offsetsArray;
  offsetsArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells + 1));
  vtkNew<vtkIdTypeArray> connArray;
  connArray->SetNumberOfTuples(static_cast<vtkIdType>(maxSize));

  if (typesArray)
  {
    typesArray->SetNumberOfComponents(1);
    typesArray->SetNumberOfTuples(static_cast<vtkIdType>(numCells));
  }

  vtkIdType* connIter = connArray->GetPointer(0);
  const vtkIdType* connBegin = connIter;

  for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
    const viskores::Id npts = cellset->GetNumberOfPointsInCell(cellId);
    assert(npts <= 8 && "Initial allocation assumes no more than 8 pts/cell.");

    const vtkIdType offset = static_cast<vtkIdType>(connIter - connBegin);
    offsetsArray->SetValue(vtkCellId, offset);

    cellset->GetCellPointIds(cellId, connIter);
    connIter += npts;

    if (typesArray)
    {
      typesArray->SetValue(vtkCellId, cellset->GetCellShape(cellId));
    }
  }

  const vtkIdType connSize = static_cast<vtkIdType>(connIter - connBegin);
  offsetsArray->SetValue(static_cast<vtkIdType>(numCells), connSize);
  connArray->Resize(connSize);
  cells->SetData(offsetsArray, connArray);

  return true;
}

VTK_ABI_NAMESPACE_END
} // namespace fromvtkm
