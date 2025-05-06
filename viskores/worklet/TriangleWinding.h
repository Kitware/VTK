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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef viskores_m_worklet_TriangleWinding_h
#define viskores_m_worklet_TriangleWinding_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/worklet/MaskIndices.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/Types.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{

/**
 * This worklet ensures that triangle windings are consistent with provided
 * cell normals. The triangles are wound CCW around the cell normals, and
 * all other cells are ignored.
 *
 * The input cellset must be unstructured.
 */
class TriangleWinding
{
public:
  // Used by Explicit and SingleType specializations
  struct WorkletWindToCellNormals : public WorkletMapField
  {
    using ControlSignature = void(FieldIn cellNormals, FieldInOut cellPoints, WholeArrayIn coords);
    using ExecutionSignature = void(_1 cellNormal, _2 cellPoints, _3 coords);

    template <typename NormalCompType, typename CellPointsType, typename CoordsPortal>
    VISKORES_EXEC void operator()(const viskores::Vec<NormalCompType, 3>& cellNormal,
                                  CellPointsType& cellPoints,
                                  const CoordsPortal& coords) const
    {
      // We only care about triangles:
      if (cellPoints.GetNumberOfComponents() != 3)
      {
        return;
      }

      using NormalType = viskores::Vec<NormalCompType, 3>;

      const NormalType p0 = coords.Get(cellPoints[0]);
      const NormalType p1 = coords.Get(cellPoints[1]);
      const NormalType p2 = coords.Get(cellPoints[2]);
      const NormalType v01 = p1 - p0;
      const NormalType v02 = p2 - p0;
      const NormalType triangleNormal = viskores::Cross(v01, v02);
      if (viskores::Dot(cellNormal, triangleNormal) < 0)
      {
        // Can't just use std::swap from exec function:
        const viskores::Id tmp = cellPoints[1];
        cellPoints[1] = cellPoints[2];
        cellPoints[2] = tmp;
      }
    }
  };

  // Used by generic implementations:
  struct WorkletGetCellShapesAndSizes : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cells, FieldOutCell shapes, FieldOutCell sizes);
    using ExecutionSignature = void(CellShape, PointCount, _2, _3);

    template <typename CellShapeTag>
    VISKORES_EXEC void operator()(const CellShapeTag cellShapeIn,
                                  const viskores::IdComponent cellSizeIn,
                                  viskores::UInt8& cellShapeOut,
                                  viskores::IdComponent& cellSizeOut) const
    {
      cellSizeOut = cellSizeIn;
      cellShapeOut = cellShapeIn.Id;
    }
  };

  struct WorkletWindToCellNormalsGeneric : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellsIn,
                                  WholeArrayIn coords,
                                  FieldInCell cellNormals,
                                  FieldOutCell cellsOut);
    using ExecutionSignature = void(PointIndices, _2, _3, _4);

    template <typename InputIds, typename Coords, typename Normal, typename OutputIds>
    VISKORES_EXEC void operator()(const InputIds& inputIds,
                                  const Coords& coords,
                                  const Normal& normal,
                                  OutputIds& outputIds) const
    {
      VISKORES_ASSERT(inputIds.GetNumberOfComponents() == outputIds.GetNumberOfComponents());

      // We only care about triangles:
      if (inputIds.GetNumberOfComponents() != 3)
      {
        // Just passthrough non-triangles
        // Cannot just assign here, must do a manual component-wise copy to
        // support VecFromPortal:
        for (viskores::IdComponent i = 0; i < inputIds.GetNumberOfComponents(); ++i)
        {
          outputIds[i] = inputIds[i];
        }
        return;
      }

      const Normal p0 = coords.Get(inputIds[0]);
      const Normal p1 = coords.Get(inputIds[1]);
      const Normal p2 = coords.Get(inputIds[2]);
      const Normal v01 = p1 - p0;
      const Normal v02 = p2 - p0;
      const Normal triangleNormal = viskores::Cross(v01, v02);
      if (viskores::Dot(normal, triangleNormal) < 0)
      { // Reorder triangle:
        outputIds[0] = inputIds[0];
        outputIds[1] = inputIds[2];
        outputIds[2] = inputIds[1];
      }
      else
      { // passthrough:
        outputIds[0] = inputIds[0];
        outputIds[1] = inputIds[1];
        outputIds[2] = inputIds[2];
      }
    }
  };

  struct Launcher
  {
    viskores::cont::UnknownCellSet Result;

    // Generic handler:
    template <typename CellSetType, typename CoordsType, typename CellNormalsType>
    VISKORES_CONT void operator()(const CellSetType& cellSet,
                                  const CoordsType& coords,
                                  const CellNormalsType& cellNormals,
                                  ...)
    {
      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      viskores::cont::Invoker invoker;

      // Get each cell's size:
      viskores::cont::ArrayHandle<viskores::IdComponent> numIndices;
      viskores::cont::ArrayHandle<viskores::UInt8> cellShapes;
      {
        WorkletGetCellShapesAndSizes worklet;
        invoker(worklet, cellSet, cellShapes, numIndices);
      }

      // Check to see if we can use CellSetSingleType:
      viskores::IdComponent cellSize = 0; // 0 if heterogeneous, >0 if homogeneous
      viskores::UInt8 cellShape = 0;      // only valid if homogeneous
      {
        auto rangeHandleSizes = viskores::cont::ArrayRangeCompute(numIndices);
        auto rangeHandleShapes = viskores::cont::ArrayRangeCompute(cellShapes);

        cellShapes.ReleaseResourcesExecution();

        auto rangeSizes = rangeHandleSizes.ReadPortal().Get(0);
        auto rangeShapes = rangeHandleShapes.ReadPortal().Get(0);

        const bool sameSize = viskores::Abs(rangeSizes.Max - rangeSizes.Min) < 0.5;
        const bool sameShape = viskores::Abs(rangeShapes.Max - rangeShapes.Min) < 0.5;

        if (sameSize && sameShape)
        {
          cellSize = static_cast<viskores::IdComponent>(rangeSizes.Min + 0.5);
          cellShape = static_cast<viskores::UInt8>(rangeShapes.Min + 0.5);
        }
      }

      if (cellSize > 0)
      { // Single cell type:
        // don't need these anymore:
        numIndices.ReleaseResources();
        cellShapes.ReleaseResources();

        viskores::cont::ArrayHandle<viskores::Id> conn;
        conn.Allocate(cellSize * numCells);

        auto offsets =
          viskores::cont::make_ArrayHandleCounting<viskores::Id>(0, cellSize, numCells);
        auto connGroupVec = viskores::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

        WorkletWindToCellNormalsGeneric worklet;
        invoker(worklet, cellSet, coords, cellNormals, connGroupVec);

        viskores::cont::CellSetSingleType<> outCells;
        outCells.Fill(cellSet.GetNumberOfPoints(), cellShape, cellSize, conn);
        this->Result = outCells;
      }
      else
      { // Multiple cell types:
        viskores::cont::ArrayHandle<viskores::Id> offsets;
        viskores::Id connSize;
        viskores::cont::ConvertNumComponentsToOffsets(numIndices, offsets, connSize);
        numIndices.ReleaseResourcesExecution();

        viskores::cont::ArrayHandle<viskores::Id> conn;
        conn.Allocate(connSize);

        // Trim the last value off for the group vec array:
        auto connGroupVec = viskores::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

        WorkletWindToCellNormalsGeneric worklet;
        invoker(worklet, cellSet, coords, cellNormals, connGroupVec);

        viskores::cont::CellSetExplicit<> outCells;
        outCells.Fill(cellSet.GetNumberOfPoints(), cellShapes, conn, offsets);
        this->Result = outCells;
      }
    }

    // Specialization for CellSetExplicit
    template <typename S, typename C, typename O, typename CoordsType, typename CellNormalsType>
    VISKORES_CONT void operator()(const viskores::cont::CellSetExplicit<S, C, O>& cellSet,
                                  const CoordsType& coords,
                                  const CellNormalsType& cellNormals,
                                  int)
    {
      using WindToCellNormals = viskores::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      viskores::cont::ArrayHandle<viskores::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                          viskores::TopologyElementTagPoint{});
        viskores::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets = cellSet.GetOffsetsArray(viskores::TopologyElementTagCell{},
                                                    viskores::TopologyElementTagPoint{});
      auto cells = viskores::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

      WindToCellNormals dispatcher;
      dispatcher.Invoke(cellNormals, cells, coords);

      const auto& shapes = cellSet.GetShapesArray(viskores::TopologyElementTagCell{},
                                                  viskores::TopologyElementTagPoint{});
      viskores::cont::CellSetExplicit<S, viskores::cont::StorageTagBasic, O> newCells;
      newCells.Fill(cellSet.GetNumberOfPoints(), shapes, conn, offsets);

      this->Result = newCells;
    }

    // Specialization for CellSetSingleType
    template <typename C, typename CoordsType, typename CellNormalsType>
    VISKORES_CONT void operator()(const viskores::cont::CellSetSingleType<C>& cellSet,
                                  const CoordsType& coords,
                                  const CellNormalsType& cellNormals,
                                  int)
    {
      using WindToCellNormals = viskores::worklet::DispatcherMapField<WorkletWindToCellNormals>;

      const auto numCells = cellSet.GetNumberOfCells();
      if (numCells == 0)
      {
        this->Result = cellSet;
        return;
      }

      viskores::cont::ArrayHandle<viskores::Id> conn;
      {
        const auto& connIn = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                          viskores::TopologyElementTagPoint{});
        viskores::cont::Algorithm::Copy(connIn, conn);
      }

      const auto& offsets = cellSet.GetOffsetsArray(viskores::TopologyElementTagCell{},
                                                    viskores::TopologyElementTagPoint{});
      auto cells = viskores::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

      WindToCellNormals dispatcher;
      dispatcher.Invoke(cellNormals, cells, coords);

      viskores::cont::CellSetSingleType<viskores::cont::StorageTagBasic> newCells;
      newCells.Fill(cellSet.GetNumberOfPoints(),
                    cellSet.GetCellShape(0),
                    cellSet.GetNumberOfPointsInCell(0),
                    conn);

      this->Result = newCells;
    }
  };

  template <typename CellSetType, typename CoordsType, typename CellNormalsType>
  VISKORES_CONT static viskores::cont::UnknownCellSet Run(const CellSetType& cellSet,
                                                          const CoordsType& coords,
                                                          const CellNormalsType& cellNormals)
  {
    Launcher launcher;
    // The last arg is just to help with overload resolution on the templated
    // Launcher::operator() method, so that the more specialized impls are
    // preferred over the generic one.
    viskores::cont::CastAndCall(cellSet, launcher, coords, cellNormals, 0);
    return launcher.Result;
  }
};
}
} // end namespace viskores::worklet

#endif // viskores_m_worklet_TriangleWinding_h
