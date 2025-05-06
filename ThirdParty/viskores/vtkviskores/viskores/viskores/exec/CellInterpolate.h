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
#ifndef viskores_exec_Interpolate_h
#define viskores_exec_Interpolate_h

#include <viskores/CellShape.h>
#include <viskores/ErrorCode.h>
#include <viskores/VecAxisAlignedPointCoordinates.h>
#include <viskores/VecFromPortalPermute.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/exec/FunctorBase.h>

#include <lcl/lcl.h>

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

namespace viskores
{
namespace exec
{

namespace internal
{

template <typename VtkcCellShapeTag, typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolateImpl(VtkcCellShapeTag tag,
                                                      const FieldVecType& field,
                                                      const ParametricCoordType& pcoords,
                                                      typename FieldVecType::ComponentType& result)
{
  if (tag.numberOfPoints() != field.GetNumberOfComponents())
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  using FieldValueType = typename FieldVecType::ComponentType;
  IdComponent numComponents = viskores::VecTraits<FieldValueType>::GetNumberOfComponents(field[0]);
  auto status =
    lcl::interpolate(tag, lcl::makeFieldAccessorNestedSOA(field, numComponents), pcoords, result);
  return viskores::internal::LclErrorToViskoresError(status);
}

} // namespace internal

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType, typename CellShapeTag>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const FieldVecType& pointFieldValues,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag tag,
  typename FieldVecType::ComponentType& result)
{
  auto lclTag =
    viskores::internal::make_LclCellShapeTag(tag, pointFieldValues.GetNumberOfComponents());
  return internal::CellInterpolateImpl(lclTag, pointFieldValues, pcoords, result);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(const FieldVecType&,
                                                  const viskores::Vec<ParametricCoordType, 3>&,
                                                  viskores::CellShapeTagEmpty,
                                                  typename FieldVecType::ComponentType& result)
{
  result = { 0 };
  return viskores::ErrorCode::OperationOnEmptyCell;
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const FieldVecType& field,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagPolyLine,
  typename FieldVecType::ComponentType& result)
{
  const viskores::IdComponent numPoints = field.GetNumberOfComponents();
  if (numPoints < 1)
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  if (numPoints == 1)
  {
    return CellInterpolate(field, pcoords, viskores::CellShapeTagVertex(), result);
  }

  using T = ParametricCoordType;

  T dt = 1 / static_cast<T>(numPoints - 1);
  viskores::IdComponent idx = static_cast<viskores::IdComponent>(pcoords[0] / dt);
  if (idx == numPoints - 1)
  {
    result = field[numPoints - 1];
    return viskores::ErrorCode::Success;
  }

  T pc = (pcoords[0] - static_cast<T>(idx) * dt) / dt;
  return internal::CellInterpolateImpl(
    lcl::Line{}, viskores::make_Vec(field[idx], field[idx + 1]), &pc, result);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const FieldVecType& field,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagPolygon,
  typename FieldVecType::ComponentType& result)
{
  const viskores::IdComponent numPoints = field.GetNumberOfComponents();
  if (numPoints < 1)
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  switch (numPoints)
  {
    case 1:
      return CellInterpolate(field, pcoords, viskores::CellShapeTagVertex(), result);
    case 2:
      return CellInterpolate(field, pcoords, viskores::CellShapeTagLine(), result);
    default:
      return internal::CellInterpolateImpl(lcl::Polygon(numPoints), field, pcoords, result);
  }
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const viskores::VecAxisAlignedPointCoordinates<2>& field,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagQuad,
  viskores::Vec3f& result)
{
  return internal::CellInterpolateImpl(lcl::Pixel{}, field, pcoords, result);
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const viskores::VecAxisAlignedPointCoordinates<3>& field,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagHexahedron,
  viskores::Vec3f& result)
{
  return internal::CellInterpolateImpl(lcl::Voxel{}, field, pcoords, result);
}

//-----------------------------------------------------------------------------
/// \brief Interpolate a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a location within the cell, interpolates the field to that location.
///
/// @param[in]  pointFieldValues A list of field values for each point in the cell. This
///     usually comes from a `FieldInPoint` argument in a
///     `viskores::worklet::WorkletVisitCellsWithPoints`.
/// @param[in]  parametricCoords The parametric coordinates where you want to get the interpolated
///     field value for.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] result Value to store the interpolated field.
template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const FieldVecType& pointFieldValues,
  const viskores::Vec<ParametricCoordType, 3>& parametricCoords,
  viskores::CellShapeTagGeneric shape,
  typename FieldVecType::ComponentType& result)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(
      status = CellInterpolate(pointFieldValues, parametricCoords, CellShapeTag(), result));
    default:
      result = { 0 };
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

//-----------------------------------------------------------------------------
/// @brief Interpolate a point field in a cell.
///
/// Given the indices of the points for each node in a `Vec`, a portal to the point
/// field values, and the parametric coordinates of a location within the cell, interpolates
/// to that location.
///
/// @param[in]  pointIndices A list of point indices for each point in the cell. This
///     usually comes from a `GetIndices()` call on the structure object provided by
///     a `WholeCellSetIn` argument to a worklet.
/// @param[in]  pointFieldPortal An array portal containing all the values in a point
///     field array. This usually comes from a `WholeArrayIn` worklet argument.
/// @param[in]  parametricCoords The parametric coordinates where you want to get the
///     interpolaged field value for.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
/// @param[out] result Value to store the interpolated field.
template <typename IndicesVecType,
          typename FieldPortalType,
          typename ParametricCoordType,
          typename CellShapeTag>
VISKORES_EXEC viskores::ErrorCode CellInterpolate(
  const IndicesVecType& pointIndices,
  const FieldPortalType& pointFieldPortal,
  const viskores::Vec<ParametricCoordType, 3>& parametricCoords,
  CellShapeTag shape,
  typename FieldPortalType::ValueType& result)
{
  return CellInterpolate(viskores::make_VecFromPortalPermute(&pointIndices, pointFieldPortal),
                         parametricCoords,
                         shape,
                         result);
}

}
} // namespace viskores::exec

#if (defined(VISKORES_GCC) || defined(VISKORES_CLANG))
#pragma GCC diagnostic pop
#endif // gcc || clang

#endif //viskores_exec_Interpolate_h
