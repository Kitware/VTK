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

#ifndef viskores_worklet_TriangulateExplicit_h
#define viskores_worklet_TriangulateExplicit_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/worklet/internal/TriangulateTables.h>

namespace viskores
{
namespace worklet
{

/// \brief Compute the triangulate cells for an explicit grid data set
class TriangulateExplicit
{
public:
  TriangulateExplicit() {}

  //
  // Worklet to count the number of triangles generated per cell
  //
  class TrianglesPerCell : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cells, ExecObject tables, FieldOut triangleCount);
    using ExecutionSignature = _3(CellShape, IncidentElementCount, _2);
    using InputDomain = _1;

    VISKORES_CONT
    TrianglesPerCell() {}

    template <typename CellShapeTag>
    VISKORES_EXEC viskores::IdComponent operator()(
      CellShapeTag shape,
      viskores::IdComponent numPoints,
      const viskores::worklet::internal::TriangulateTablesExecutionObject& tables) const
    {
      return tables.GetCount(shape, numPoints);
    }
  };

  //
  // Worklet to turn cells into triangles
  // Vertices remain the same and each cell is processed with needing topology
  //
  class TriangulateCell : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  ExecObject tables,
                                  FieldOutCell connectivityOut);
    using ExecutionSignature = void(CellShape, PointIndices, _2, _3, VisitIndex);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename CountArrayType>
    VISKORES_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    // Each cell produces triangles and write result at the offset
    template <typename CellShapeTag, typename ConnectivityInVec, typename ConnectivityOutVec>
    VISKORES_EXEC void operator()(
      CellShapeTag shape,
      const ConnectivityInVec& connectivityIn,
      const viskores::worklet::internal::TriangulateTablesExecutionObject& tables,
      ConnectivityOutVec& connectivityOut,
      viskores::IdComponent visitIndex) const
    {
      viskores::IdComponent3 triIndices = tables.GetIndices(shape, visitIndex);
      connectivityOut[0] = connectivityIn[triIndices[0]];
      connectivityOut[1] = connectivityIn[triIndices[1]];
      connectivityOut[2] = connectivityIn[triIndices[2]];
    }
  };
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(
    const CellSetType& cellSet,
    viskores::cont::ArrayHandle<viskores::IdComponent>& outCellsPerCell)
  {
    viskores::cont::CellSetSingleType<> outCellSet;

    viskores::cont::Invoker invoke;

    // Output topology
    viskores::cont::ArrayHandle<viskores::Id> outConnectivity;

    viskores::worklet::internal::TriangulateTables tables;

    // Determine the number of output cells each input cell will generate
    invoke(TrianglesPerCell{}, cellSet, tables, outCellsPerCell);

    // Build new cells
    invoke(TriangulateCell{},
           TriangulateCell::MakeScatter(outCellsPerCell),
           cellSet,
           tables,
           viskores::cont::make_ArrayHandleGroupVec<3>(outConnectivity));

    // Add cells to output cellset
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), viskores::CellShapeTagTriangle::Id, 3, outConnectivity);
    return outCellSet;
  }
};

}
} // namespace viskores::worklet

#endif // viskores_worklet_TriangulateExplicit_h
