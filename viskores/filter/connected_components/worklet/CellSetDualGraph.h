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
#ifndef viskores_worklet_connectivity_CellSetDualGraph_h
#define viskores_worklet_connectivity_CellSetDualGraph_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/exec/CellEdge.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
namespace connectivity
{
namespace detail
{
struct EdgeCount : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell numEdgesInCell);

  using ExecutionSignature = void(CellShape, PointCount, _2);

  using InputDomain = _1;

  template <typename CellShapeTag>
  VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                viskores::IdComponent pointCount,
                                viskores::IdComponent& numEdges) const
  {
    viskores::exec::CellEdgeNumberOfEdges(pointCount, cellShape, numEdges);
  }
};

struct EdgeExtract : public viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn, FieldOutCell cellIndices, FieldOutCell edgeIndices);

  using ExecutionSignature = void(CellShape, InputIndex, PointIndices, VisitIndex, _2, _3);

  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template <typename CellShapeTag,
            typename CellIndexType,
            typename PointIndexVecType,
            typename EdgeIndexVecType>
  VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                CellIndexType cellIndex,
                                const PointIndexVecType& pointIndices,
                                viskores::IdComponent visitIndex,
                                CellIndexType& cellIndexOut,
                                EdgeIndexVecType& edgeIndices) const
  {
    cellIndexOut = cellIndex;
    viskores::exec::CellEdgeCanonicalId(
      pointIndices.GetNumberOfComponents(), visitIndex, cellShape, pointIndices, edgeIndices);
  }
};

struct CellToCellConnectivity : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn index,
                                WholeArrayIn cells,
                                WholeArrayOut from,
                                WholeArrayOut to);

  using ExecutionSignature = void(_1, InputIndex, _2, _3, _4);

  using InputDomain = _1;

  template <typename ConnectivityPortalType, typename CellIdPortalType>
  VISKORES_EXEC void operator()(viskores::Id offset,
                                viskores::Id index,
                                const CellIdPortalType& cells,
                                ConnectivityPortalType& from,
                                ConnectivityPortalType& to) const
  {
    from.Set(index * 2, cells.Get(offset));
    to.Set(index * 2, cells.Get(offset + 1));
    from.Set(index * 2 + 1, cells.Get(offset + 1));
    to.Set(index * 2 + 1, cells.Get(offset));
  }
};
} // viskores::worklet::connectivity::detail

class CellSetDualGraph
{
  using Algorithm = viskores::cont::Algorithm;

  static void EdgeToCellConnectivity(const viskores::cont::UnknownCellSet& cellSet,
                                     viskores::cont::ArrayHandle<viskores::Id>& cellIds,
                                     viskores::cont::ArrayHandle<viskores::Id2>& cellEdges)
  {
    // Get number of edges for each cell and use it as scatter count.
    viskores::cont::ArrayHandle<viskores::IdComponent> numEdgesPerCell;
    viskores::worklet::DispatcherMapTopology<detail::EdgeCount> edgesPerCellDisp;
    edgesPerCellDisp.Invoke(cellSet, numEdgesPerCell);

    // Get uncompress Cell to Edge mapping
    viskores::worklet::ScatterCounting scatter{ numEdgesPerCell };
    viskores::worklet::DispatcherMapTopology<detail::EdgeExtract> edgeExtractDisp{ scatter };
    edgeExtractDisp.Invoke(cellSet, cellIds, cellEdges);
  }

public:
  struct degree2
  {
    VISKORES_EXEC
    bool operator()(viskores::Id degree) const { return degree >= 2; }
  };

  static void Run(const viskores::cont::UnknownCellSet& cellSet,
                  viskores::cont::ArrayHandle<viskores::Id>& numIndicesArray,
                  viskores::cont::ArrayHandle<viskores::Id>& indexOffsetArray,
                  viskores::cont::ArrayHandle<viskores::Id>& connectivityArray)
  {
    // calculate the uncompressed Edge to Cell connectivity from Point to Cell connectivity
    // in the CellSet
    viskores::cont::ArrayHandle<viskores::Id> cellIds;
    viskores::cont::ArrayHandle<viskores::Id2> cellEdges;
    EdgeToCellConnectivity(cellSet, cellIds, cellEdges);

    // sort cell ids by cell edges, this groups cells by cell edges
    Algorithm::SortByKey(cellEdges, cellIds);

    // count how many times an edge is shared by cells.
    viskores::cont::ArrayHandle<viskores::Id2> uniqueEdges;
    viskores::cont::ArrayHandle<viskores::Id> uniqueEdgeDegree;
    Algorithm::ReduceByKey(
      cellEdges,
      viskores::cont::ArrayHandleConstant<viskores::Id>(1, cellEdges.GetNumberOfValues()),
      uniqueEdges,
      uniqueEdgeDegree,
      viskores::Add());

    // Extract edges shared by two cells
    viskores::cont::ArrayHandle<viskores::Id2> sharedEdges;
    Algorithm::CopyIf(uniqueEdges, uniqueEdgeDegree, sharedEdges, degree2());

    // find shared edges within all the edges.
    viskores::cont::ArrayHandle<viskores::Id> lb;
    Algorithm::LowerBounds(cellEdges, sharedEdges, lb);

    // take each shared edge and the cells to create 2 edges of the dual graph
    viskores::cont::ArrayHandle<viskores::Id> connFrom;
    viskores::cont::ArrayHandle<viskores::Id> connTo;
    connFrom.Allocate(sharedEdges.GetNumberOfValues() * 2);
    connTo.Allocate(sharedEdges.GetNumberOfValues() * 2);
    viskores::worklet::DispatcherMapField<detail::CellToCellConnectivity> c2cDisp;
    c2cDisp.Invoke(lb, cellIds, connFrom, connTo);

    // Turn dual graph into Compressed Sparse Row format
    Algorithm::SortByKey(connFrom, connTo);
    Algorithm::Copy(connTo, connectivityArray);

    viskores::cont::ArrayHandle<viskores::Id> dualGraphVertices;
    Algorithm::ReduceByKey(
      connFrom,
      viskores::cont::ArrayHandleConstant<viskores::Id>(1, connFrom.GetNumberOfValues()),
      dualGraphVertices,
      numIndicesArray,
      viskores::Add());
    Algorithm::ScanExclusive(numIndicesArray, indexOffsetArray);
  }
};
}
}
}

#endif //viskores_worklet_connectivity_CellSetDualGraph_h
