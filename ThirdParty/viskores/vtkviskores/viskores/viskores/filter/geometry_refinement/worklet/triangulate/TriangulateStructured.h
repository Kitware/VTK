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

#ifndef viskores_worklet_TriangulateStructured_h
#define viskores_worklet_TriangulateStructured_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Field.h>

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{
namespace triangulate
{
//
// Worklet to turn quads into triangles
// Vertices remain the same and each cell is processed with needing topology
//
class TriangulateCell : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, VisitIndex);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterUniform<2>;

  // Each quad cell produces 2 triangle cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec>
  VISKORES_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                                ConnectivityOutVec& connectivityOut,
                                viskores::IdComponent visitIndex) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::IdComponent StructuredTriangleIndices[2][3] = {
      { 0, 1, 2 }, { 0, 2, 3 }
    };
    connectivityOut[0] = connectivityIn[StructuredTriangleIndices[visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTriangleIndices[visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTriangleIndices[visitIndex][2]];
  }
};
}

/// \brief Compute the triangulate cells for a uniform grid data set
class TriangulateStructured
{
public:
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(
    const CellSetType& cellSet,
    viskores::cont::ArrayHandle<viskores::IdComponent>& outCellsPerCell)

  {
    viskores::cont::CellSetSingleType<> outCellSet;
    viskores::cont::ArrayHandle<viskores::Id> connectivity;

    viskores::worklet::DispatcherMapTopology<triangulate::TriangulateCell> dispatcher;
    dispatcher.Invoke(cellSet, viskores::cont::make_ArrayHandleGroupVec<3>(connectivity));

    // Fill in array of output cells per input cell
    viskores::cont::ArrayCopy(
      viskores::cont::ArrayHandleConstant<viskores::IdComponent>(2, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), viskores::CellShapeTagTriangle::Id, 3, connectivity);
    return outCellSet;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_TriangulateStructured_h
