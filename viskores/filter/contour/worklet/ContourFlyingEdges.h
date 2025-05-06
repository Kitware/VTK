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

#ifndef viskores_worklet_ContourFlyingEdges_h
#define viskores_worklet_ContourFlyingEdges_h


#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/filter/contour/worklet/contour/CommonState.h>
#include <viskores/filter/contour/worklet/contour/FieldPropagation.h>
#include <viskores/filter/contour/worklet/contour/FlyingEdges.h>

namespace viskores
{
namespace worklet
{

/// \brief Compute the isosurface of a given \c CellSetStructured<3> input with
/// \c ArrayHandleUniformPointCoordinates for point coordinates using the Flying Edges algorithm.
class ContourFlyingEdges
{
public:
  //----------------------------------------------------------------------------
  ContourFlyingEdges(bool mergeDuplicates = true)
    : SharedState(mergeDuplicates)
  {
  }

  //----------------------------------------------------------------------------
  viskores::cont::ArrayHandle<viskores::Id2> GetInterpolationEdgeIds() const
  {
    return this->SharedState.InterpolationEdgeIds;
  }

  //----------------------------------------------------------------------------
  void SetMergeDuplicatePoints(bool merge) { this->SharedState.MergeDuplicatePoints = merge; }

  //----------------------------------------------------------------------------
  bool GetMergeDuplicatePoints() const { return this->SharedState.MergeDuplicatePoints; }

  //----------------------------------------------------------------------------
  viskores::cont::ArrayHandle<viskores::Id> GetCellIdMap() const
  {
    return this->SharedState.CellIdMap;
  }

  //----------------------------------------------------------------------------
  template <typename InArrayType, typename OutArrayType>
  void ProcessPointField(const InArrayType& input, const OutArrayType& output) const
  {

    using viskores::worklet::contour::MapPointField;
    viskores::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher;

    applyFieldDispatcher.Invoke(this->SharedState.InterpolationEdgeIds,
                                this->SharedState.InterpolationWeights,
                                input,
                                output);
  }

  //----------------------------------------------------------------------------
  void ReleaseCellMapArrays() { this->SharedState.CellIdMap.ReleaseResources(); }

  // Filter called without normals generation
  template <typename IVType,
            typename ValueType,
            typename CoordsType,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  viskores::cont::CellSetSingleType<> Run(
    const std::vector<IVType>& isovalues,
    const viskores::cont::CellSetStructured<3>& cells,
    const CoordsType& coordinateSystem,
    const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateType, 3>, StorageTagVertices>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateType, 3>> normals;

    viskores::cont::CellSetSingleType<> outputCells;
    return flying_edges::execute(
      cells, coordinateSystem, isovalues, input, vertices, normals, this->SharedState);
  }

  // Filter called with normals generation
  template <typename IVType,
            typename ValueType,
            typename CoordsType,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals>
  viskores::cont::CellSetSingleType<> Run(
    const std::vector<IVType>& isovalues,
    const viskores::cont::CellSetStructured<3>& cells,
    const CoordsType& coordinateSystem,
    const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateType, 3>, StorageTagVertices>& vertices,
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateType, 3>, StorageTagNormals>& normals)
  {
    this->SharedState.GenerateNormals = true;
    viskores::cont::CellSetSingleType<> outputCells;
    return flying_edges::execute(
      cells, coordinateSystem, isovalues, input, vertices, normals, this->SharedState);
  }

private:
  viskores::worklet::contour::CommonState SharedState;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_ContourFlyingEdges_h
