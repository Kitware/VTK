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

#ifndef viskores_worklet_ContourMarchingCells_h
#define viskores_worklet_ContourMarchingCells_h

#include <viskores/filter/contour/viskores_filter_contour_export.h>
#include <viskores/filter/contour/worklet/contour/CommonState.h>
#include <viskores/filter/contour/worklet/contour/FieldPropagation.h>
#include <viskores/filter/contour/worklet/contour/MarchingCells.h>

#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/UnknownCellSet.h>

#include <viskores/internal/Instantiations.h>


namespace viskores
{
namespace worklet
{
namespace contour
{

template <viskores::UInt8 Dims>
struct DeduceCoordType
{
  template <typename CoordinateType, typename CellSetType, typename... Args>
  void operator()(const CoordinateType& coords,
                  const CellSetType& cells,
                  viskores::cont::CellSetSingleType<>& result,
                  Args&&... args) const
  {
    result = marching_cells::execute<Dims>(cells, coords, std::forward<Args>(args)...);
  }
};

template <viskores::UInt8 Dims>
struct DeduceCellType
{
  template <typename CellSetType, typename ValueType, typename StorageTagField>
  void operator()(const CellSetType& cells,
                  const viskores::cont::CoordinateSystem& coordinateSystem,
                  viskores::cont::CellSetSingleType<>& outputCells,
                  const std::vector<ValueType>& isovalues,
                  const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
                  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
                  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
                  viskores::worklet::contour::CommonState& sharedState) const;
};

// Declared outside of class, non-inline so that instantiations can be exported correctly.
template <viskores::UInt8 Dims>
template <typename CellSetType, typename ValueType, typename StorageTagField>
void DeduceCellType<Dims>::operator()(
  const CellSetType& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<ValueType>& isovalues,
  const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const
{
  viskores::cont::CastAndCall(coordinateSystem,
                              contour::DeduceCoordType<Dims>{},
                              cells,
                              outputCells,
                              isovalues,
                              input,
                              vertices,
                              normals,
                              sharedState);
}

} // namespace contour

/// \brief Compute the isosurface of a given 3D data set, supports all linear cell types
class ContourMarchingCells
{
public:
  //----------------------------------------------------------------------------
  ContourMarchingCells(bool mergeDuplicates = true)
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

public:
  // Filter called without normals generation
  template <viskores::UInt8 Dims, typename ValueType, typename StorageTagField>
  VISKORES_CONT viskores::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const viskores::cont::UnknownCellSet& cells,
    const viskores::cont::CoordinateSystem& coordinateSystem,
    const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
    viskores::cont::ArrayHandle<viskores::Vec3f>& vertices)
  {
    this->SharedState.GenerateNormals = false;
    viskores::cont::ArrayHandle<viskores::Vec3f> normals;

    viskores::cont::CellSetSingleType<> outputCells;
    viskores::cont::CastAndCall(cells,
                                contour::DeduceCellType<Dims>{},
                                coordinateSystem,
                                outputCells,
                                isovalues,
                                input,
                                vertices,
                                normals,
                                this->SharedState);
    return outputCells;
  }

  // Filter called with normals generation
  template <viskores::UInt8 Dims, typename ValueType, typename StorageTagField>
  VISKORES_CONT viskores::cont::CellSetSingleType<> Run(
    const std::vector<ValueType>& isovalues,
    const viskores::cont::UnknownCellSet& cells,
    const viskores::cont::CoordinateSystem& coordinateSystem,
    const viskores::cont::ArrayHandle<ValueType, StorageTagField>& input,
    viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
    viskores::cont::ArrayHandle<viskores::Vec3f>& normals)
  {
    this->SharedState.GenerateNormals = true;

    viskores::cont::CellSetSingleType<> outputCells;
    viskores::cont::CastAndCall(cells,
                                contour::DeduceCellType<Dims>{},
                                coordinateSystem,
                                outputCells,
                                isovalues,
                                input,
                                vertices,
                                normals,
                                this->SharedState);
    return outputCells;
  }


private:
  viskores::worklet::contour::CommonState SharedState;
};

}
} // namespace viskores::worklet

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetStructured<2>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetStructured<2>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetStructured<3>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetStructured<3>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<3>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<2>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<1>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<1>::operator()(
  const viskores::cont::CellSetExplicit<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<1>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float32>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float32, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END
VISKORES_INSTANTIATION_BEGIN
extern template void viskores::worklet::contour::DeduceCellType<1>::operator()(
  const viskores::cont::CellSetSingleType<>& cells,
  const viskores::cont::CoordinateSystem& coordinateSystem,
  viskores::cont::CellSetSingleType<>& outputCells,
  const std::vector<viskores::Float64>& isovalues,
  const viskores::cont::ArrayHandle<viskores::Float64, viskores::cont::StorageTagBasic>& input,
  viskores::cont::ArrayHandle<viskores::Vec3f>& vertices,
  viskores::cont::ArrayHandle<viskores::Vec3f>& normals,
  viskores::worklet::contour::CommonState& sharedState) const;
VISKORES_INSTANTIATION_END

#endif // viskores_worklet_ContourMarchingCells_h
