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

#ifndef viskores_worklet_TetrahedralizeStructured_h
#define viskores_worklet_TetrahedralizeStructured_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Field.h>

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

namespace tetrahedralize
{
//
// Worklet to turn hexahedra into tetrahedra
// Vertices remain the same and each cell is processed with needing topology
//
class TetrahedralizeCell : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldOutCell connectivityOut);
  using ExecutionSignature = void(PointIndices, _2, ThreadIndices);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterUniform<5>;

  // Each hexahedron cell produces five tetrahedron cells
  template <typename ConnectivityInVec, typename ConnectivityOutVec, typename ThreadIndicesType>
  VISKORES_EXEC void operator()(const ConnectivityInVec& connectivityIn,
                                ConnectivityOutVec& connectivityOut,
                                const ThreadIndicesType threadIndices) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::IdComponent StructuredTetrahedronIndices[2][5][4] = {
      { { 0, 1, 3, 4 }, { 1, 4, 5, 6 }, { 1, 4, 6, 3 }, { 1, 3, 6, 2 }, { 3, 6, 7, 4 } },
      { { 2, 1, 5, 0 }, { 0, 2, 3, 7 }, { 2, 5, 6, 7 }, { 0, 7, 4, 5 }, { 0, 2, 7, 5 } }
    };

    viskores::Id3 inputIndex = threadIndices.GetInputIndex3D();

    // Calculate the type of tetrahedron generated because it alternates
    viskores::Id indexType = (inputIndex[0] + inputIndex[1] + inputIndex[2]) % 2;

    viskores::IdComponent visitIndex = threadIndices.GetVisitIndex();

    connectivityOut[0] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][0]];
    connectivityOut[1] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][1]];
    connectivityOut[2] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][2]];
    connectivityOut[3] = connectivityIn[StructuredTetrahedronIndices[indexType][visitIndex][3]];
  }
};
}

/// \brief Compute the tetrahedralize cells for a uniform grid data set
class TetrahedralizeStructured
{
public:
  template <typename CellSetType>
  viskores::cont::CellSetSingleType<> Run(
    const CellSetType& cellSet,
    viskores::cont::ArrayHandle<viskores::IdComponent>& outCellsPerCell)
  {
    viskores::cont::CellSetSingleType<> outCellSet;
    viskores::cont::ArrayHandle<viskores::Id> connectivity;

    viskores::worklet::DispatcherMapTopology<tetrahedralize::TetrahedralizeCell> dispatcher;
    dispatcher.Invoke(cellSet, viskores::cont::make_ArrayHandleGroupVec<4>(connectivity));

    // Fill in array of output cells per input cell
    viskores::cont::ArrayCopy(
      viskores::cont::ArrayHandleConstant<viskores::IdComponent>(5, cellSet.GetNumberOfCells()),
      outCellsPerCell);

    // Add cells to output cellset
    outCellSet.Fill(cellSet.GetNumberOfPoints(), viskores::CellShapeTagTetra::Id, 4, connectivity);
    return outCellSet;
  }
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_TetrahedralizeStructured_h
