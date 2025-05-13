// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "CellSetConverters.h"

#include "ArrayConverters.hxx"
#include "DataSetConverters.h"

#include <viskores/cont/openmp/DeviceAdapterOpenMP.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>
#include <viskores/cont/tbb/DeviceAdapterTBB.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleTransform.h>
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

namespace tovtkm
{
VTK_ABI_NAMESPACE_BEGIN

namespace
{

struct ReorderHex : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  VISKORES_EXEC void operator()(viskores::Vec<viskores::Id, 8>& indices) const
  {
    viskores::Swap(indices[2], indices[3]);
    viskores::Swap(indices[6], indices[7]);
  }
};

struct BuildSingleTypeCellSetVisitor
{
  template <typename CellStateT>
  viskores::cont::UnknownCellSet operator()(CellStateT& state, viskores::UInt8 cellType,
    viskores::IdComponent cellSize, vtkIdType numPoints)
  {
    using VTKIdT = typename CellStateT::ValueType; // might not be vtkIdType...
    static constexpr bool IsVtkmIdType = std::is_same<VTKIdT, viskores::Id>::value;

    // Construct an arrayhandle that holds the connectivity array
    auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType
      ? connHandleDirect
      : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);

    using ConnHandleType = typename std::decay<decltype(connHandle)>::type;
    using ConnStorageTag = typename ConnHandleType::StorageTag;
    using CellSetType = viskores::cont::CellSetSingleType<ConnStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(static_cast<viskores::Id>(numPoints), cellType, cellSize, connHandle);
    return cellSet;
  }
};

struct BuildSingleTypeVoxelCellSetVisitor
{
  template <typename CellStateT>
  viskores::cont::UnknownCellSet operator()(CellStateT& state, vtkIdType numPoints)
  {
    viskores::cont::ArrayHandle<viskores::Id> connHandle;
    {
      auto* conn = state.GetConnectivity();
      const auto* origData = conn->GetPointer(0);
      const viskores::Id numIds = conn->GetNumberOfValues();
      viskores::cont::ArrayCopy(
        viskores::cont::make_ArrayHandle(origData, numIds, viskores::CopyFlag::Off), connHandle);

      // reorder cells from voxel->hex
      viskores::cont::Invoker invoke;
      invoke(ReorderHex{}, viskores::cont::make_ArrayHandleGroupVec<8>(connHandle));
    }

    using CellSetType = viskores::cont::CellSetSingleType<>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, viskores::CELL_SHAPE_HEXAHEDRON, 8, connHandle);
    return cellSet;
  }
};

} // end anon namespace

// convert a cell array of a single type to a viskores CellSetSingleType
viskores::cont::UnknownCellSet ConvertSingleType(
  vtkCellArray* cells, int cellType, vtkIdType numberOfPoints)
{
  switch (cellType)
  {
    case VTK_LINE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_LINE, 2, numberOfPoints);

    case VTK_HEXAHEDRON:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_HEXAHEDRON, 8, numberOfPoints);

    case VTK_VOXEL:
      // Note that this is a special case that reorders ids voxel to hex:
      return cells->Visit(BuildSingleTypeVoxelCellSetVisitor{}, numberOfPoints);

    case VTK_QUAD:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_QUAD, 4, numberOfPoints);

    case VTK_TETRA:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_TETRA, 4, numberOfPoints);

    case VTK_TRIANGLE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_TRIANGLE, 3, numberOfPoints);

    case VTK_VERTEX:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_VERTEX, 1, numberOfPoints);

    case VTK_WEDGE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_WEDGE, 6, numberOfPoints);

    case VTK_PYRAMID:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, viskores::CELL_SHAPE_PYRAMID, 5, numberOfPoints);

    default:
      break;
  }

  throw viskores::cont::ErrorBadType("Unsupported VTK cell type in "
                                     "CellSetSingleType converter.");
}

namespace
{

struct BuildExplicitCellSetVisitor
{
  template <typename CellStateT, typename S>
  viskores::cont::UnknownCellSet operator()(CellStateT& state,
    const viskores::cont::ArrayHandle<viskores::UInt8, S>& shapes, viskores::Id numPoints) const
  {
    using VTKIdT = typename CellStateT::ValueType; // might not be vtkIdType...
    static constexpr bool IsVtkmIdType = std::is_same<VTKIdT, viskores::Id>::value;

    // Construct arrayhandles to hold the arrays
    auto offsetsHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(state.GetOffsets());
    auto connHandleDirect = tovtkm::vtkAOSDataArrayToFlatArrayHandle(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType
      ? connHandleDirect
      : viskores::cont::make_ArrayHandleCast<viskores::Id>(connHandleDirect);
    auto offsetsHandle = IsVtkmIdType
      ? offsetsHandleDirect
      : viskores::cont::make_ArrayHandleCast<viskores::Id>(offsetsHandleDirect);

    using ShapesStorageTag = typename std::decay<decltype(shapes)>::type::StorageTag;
    using ConnStorageTag = typename decltype(connHandle)::StorageTag;
    using OffsetsStorageTag = typename decltype(offsetsHandle)::StorageTag;
    using CellSetType =
      viskores::cont::CellSetExplicit<ShapesStorageTag, ConnStorageTag, OffsetsStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);
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
  vtkUnsignedCharArray* types, vtkCellArray* cells, vtkIdType numberOfPoints)
{
  auto shapes = tovtkm::vtkAOSDataArrayToFlatArrayHandle(types);
  if (!viskores::cont::Algorithm::Reduce(
        viskores::cont::make_ArrayHandleTransform(shapes, SupportedCellShape{}), true,
        viskores::LogicalAnd()))
  {
    throw viskores::cont::ErrorBadType("Unsupported VTK cell type in CellSet converter.");
  }

  return cells->Visit(BuildExplicitCellSetVisitor{}, shapes, numberOfPoints);
}

VTK_ABI_NAMESPACE_END
} // namespace tovtkm

namespace fromvtkm
{
VTK_ABI_NAMESPACE_BEGIN

bool Convert(const viskores::cont::UnknownCellSet& toConvert, vtkCellArray* cells,
  vtkUnsignedCharArray* typesArray)
{
  const auto* cellset = toConvert.GetCellSetBase();

  // small hack as we can't compute properly the number of cells
  // instead we will pre-allocate and than shrink
  const viskores::Id numCells = cellset->GetNumberOfCells();
  const viskores::Id maxSize = numCells * 8; // largest cell type is hex

  // TODO this could steal the guts out of explicit cellsets as a future
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
