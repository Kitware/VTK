//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "CellSetConverters.h"

#include "ArrayConverters.hxx"
#include "DataSetConverters.h"

#include "vtkmFilterPolicy.h"

#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkUnsignedCharArray.h"

namespace tovtkm
{

namespace
{

template <typename PortalT>
struct ReorderHex : public vtkm::exec::FunctorBase
{
  ReorderHex(PortalT portal)
    : Portal{ portal }
  {
  }

  void operator()(vtkm::Id index) const
  {
    const vtkm::Id offset = index * 8;

    auto doSwap = [&](vtkm::Id id1, vtkm::Id id2) {
      id1 += offset;
      id2 += offset;

      const auto t = this->Portal.Get(id1);
      this->Portal.Set(id1, this->Portal.Get(id2));
      this->Portal.Set(id2, t);
    };

    doSwap(2, 3);
    doSwap(6, 7);
  }

  PortalT Portal;
};

struct RunReorder
{
  RunReorder(vtkm::cont::ArrayHandle<vtkm::Id>& handle)
    : Handle(handle)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    using Algo = typename vtkm::cont::DeviceAdapterAlgorithm<Device>;

    auto portal = this->Handle.PrepareForInPlace(Device{});

    using Functor = ReorderHex<decltype(portal)>;
    Functor reorder{ portal };

    Algo::Schedule(reorder, portal.GetNumberOfValues() / 8);
    return true;
  }

  vtkm::cont::ArrayHandle<vtkm::Id>& Handle;
};

struct BuildSingleTypeCellSetVisitor
{
  template <typename CellStateT>
  vtkm::cont::DynamicCellSet operator()(
    CellStateT& state, vtkm::UInt8 cellType, vtkm::IdComponent cellSize, vtkIdType numPoints)
  {
    using VTKIdT = typename CellStateT::ValueType; // might not be vtkIdType...
    using VTKArrayT = vtkAOSDataArrayTemplate<VTKIdT>;
    static constexpr bool IsVtkmIdType = std::is_same<VTKIdT, vtkm::Id>::value;

    // Construct an arrayhandle that holds the connectivity array
    using DirectConverter = tovtkm::DataArrayToArrayHandle<VTKArrayT, 1>;
    auto connHandleDirect = DirectConverter::Wrap(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType ? connHandleDirect
                                   : vtkm::cont::make_ArrayHandleCast<vtkm::Id>(connHandleDirect);

    using ConnHandleType = typename std::decay<decltype(connHandle)>::type;
    using ConnStorageTag = typename ConnHandleType::StorageTag;
    using CellSetType = vtkm::cont::CellSetSingleType<ConnStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(static_cast<vtkm::Id>(numPoints), cellType, cellSize, connHandle);
    return cellSet;
  }
};

struct BuildSingleTypeVoxelCellSetVisitor
{
  template <typename CellStateT>
  vtkm::cont::DynamicCellSet operator()(CellStateT& state, vtkIdType numPoints)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> connHandle;
    {
      auto* conn = state.GetConnectivity();
      const auto* origData = conn->GetPointer(0);
      const vtkm::Id numIds = conn->GetNumberOfValues();
      vtkm::cont::ArrayCopy(
        vtkm::cont::make_ArrayHandle(origData, numIds, vtkm::CopyFlag::Off), connHandle);

      // reorder cells from voxel->hex: which only can run on
      // devices that have shared memory / vtable with the CPU
      using SMPTypes = vtkm::List<vtkm::cont::DeviceAdapterTagTBB,
        vtkm::cont::DeviceAdapterTagOpenMP, vtkm::cont::DeviceAdapterTagSerial>;

      RunReorder reorder{ connHandle };
      vtkm::cont::TryExecute(reorder, SMPTypes{});
    }

    using CellSetType = vtkm::cont::CellSetSingleType<>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, vtkm::CELL_SHAPE_HEXAHEDRON, 8, connHandle);
    return cellSet;
  }
};

} // end anon namespace

// convert a cell array of a single type to a vtkm CellSetSingleType
vtkm::cont::DynamicCellSet ConvertSingleType(
  vtkCellArray* cells, int cellType, vtkIdType numberOfPoints)
{
  switch (cellType)
  {
    case VTK_LINE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_LINE, 2, numberOfPoints);

    case VTK_HEXAHEDRON:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_HEXAHEDRON, 8, numberOfPoints);

    case VTK_VOXEL:
      // Note that this is a special case that reorders ids voxel to hex:
      return cells->Visit(BuildSingleTypeVoxelCellSetVisitor{}, numberOfPoints);

    case VTK_QUAD:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_QUAD, 4, numberOfPoints);

    case VTK_TETRA:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_TETRA, 4, numberOfPoints);

    case VTK_TRIANGLE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_TRIANGLE, 3, numberOfPoints);

    case VTK_VERTEX:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_VERTEX, 1, numberOfPoints);

    case VTK_WEDGE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_WEDGE, 6, numberOfPoints);

    case VTK_PYRAMID:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, vtkm::CELL_SHAPE_PYRAMID, 5, numberOfPoints);

    default:
      break;
  }

  throw vtkm::cont::ErrorBadType("Unsupported VTK cell type in "
                                 "CellSetSingleType converter.");
}

namespace
{

struct BuildExplicitCellSetVisitor
{
  template <typename CellStateT, typename S>
  vtkm::cont::DynamicCellSet operator()(CellStateT& state,
    const vtkm::cont::ArrayHandle<vtkm::UInt8, S>& shapes, vtkm::Id numPoints) const
  {
    using VTKIdT = typename CellStateT::ValueType; // might not be vtkIdType...
    using VTKArrayT = vtkAOSDataArrayTemplate<VTKIdT>;
    static constexpr bool IsVtkmIdType = std::is_same<VTKIdT, vtkm::Id>::value;

    // Construct arrayhandles to hold the arrays
    using DirectConverter = tovtkm::DataArrayToArrayHandle<VTKArrayT, 1>;
    auto offsetsHandleDirect = DirectConverter::Wrap(state.GetOffsets());
    auto connHandleDirect = DirectConverter::Wrap(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType ? connHandleDirect
                                   : vtkm::cont::make_ArrayHandleCast<vtkm::Id>(connHandleDirect);
    auto offsetsHandle = IsVtkmIdType
      ? offsetsHandleDirect
      : vtkm::cont::make_ArrayHandleCast<vtkm::Id>(offsetsHandleDirect);

    using ShapesStorageTag = typename std::decay<decltype(shapes)>::type::StorageTag;
    using ConnStorageTag = typename decltype(connHandle)::StorageTag;
    using OffsetsStorageTag = typename decltype(offsetsHandle)::StorageTag;
    using CellSetType =
      vtkm::cont::CellSetExplicit<ShapesStorageTag, ConnStorageTag, OffsetsStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);
    return cellSet;
  }
};

} // end anon namespace

// convert a cell array of mixed types to a vtkm CellSetExplicit
vtkm::cont::DynamicCellSet Convert(
  vtkUnsignedCharArray* types, vtkCellArray* cells, vtkIdType numberOfPoints)
{
  using ShapeArrayType = vtkAOSDataArrayTemplate<vtkm::UInt8>;
  using ShapeConverter = tovtkm::DataArrayToArrayHandle<ShapeArrayType, 1>;
  return cells->Visit(BuildExplicitCellSetVisitor{}, ShapeConverter::Wrap(types), numberOfPoints);
}

} // namespace tovtkm

namespace fromvtkm
{

bool Convert(const vtkm::cont::DynamicCellSet& toConvert, vtkCellArray* cells,
  vtkUnsignedCharArray* typesArray)
{
  const auto* cellset = toConvert.GetCellSetBase();

  // small hack as we can't compute properly the number of cells
  // instead we will pre-allocate and than shrink
  const vtkm::Id numCells = cellset->GetNumberOfCells();
  const vtkm::Id maxSize = numCells * 8; // largest cell type is hex

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

  for (vtkm::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const vtkIdType vtkCellId = static_cast<vtkIdType>(cellId);
    const vtkm::Id npts = cellset->GetNumberOfPointsInCell(cellId);
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

} // namespace fromvtkm
