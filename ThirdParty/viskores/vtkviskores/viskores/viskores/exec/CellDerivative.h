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
#ifndef viskores_exec_Derivative_h
#define viskores_exec_Derivative_h

#include <viskores/CellShape.h>
#include <viskores/ErrorCode.h>
#include <viskores/VecAxisAlignedPointCoordinates.h>
#include <viskores/VecTraits.h>

#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/FunctorBase.h>

#include <lcl/lcl.h>

namespace viskores
{
namespace exec
{

//-----------------------------------------------------------------------------
namespace internal
{

template <typename LclCellShapeTag,
          typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivativeImpl(
  LclCellShapeTag tag,
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const ParametricCoordType& pcoords,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  result = { 0 };
  if ((field.GetNumberOfComponents() != tag.numberOfPoints()) ||
      (wCoords.GetNumberOfComponents() != tag.numberOfPoints()))
  {
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  using FieldType = typename FieldVecType::ComponentType;

  auto fieldNumComponents = viskores::VecTraits<FieldType>::GetNumberOfComponents(field[0]);
  auto status = lcl::derivative(tag,
                                lcl::makeFieldAccessorNestedSOA(wCoords, 3),
                                lcl::makeFieldAccessorNestedSOA(field, fieldNumComponents),
                                pcoords,
                                result[0],
                                result[1],
                                result[2]);
  return viskores::internal::LclErrorToViskoresError(status);
}

} // namespace internal

template <typename FieldVecType,
          typename WorldCoordType,
          typename ParametricCoordType,
          typename CellShapeTag>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  CellShapeTag shape,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  return internal::CellDerivativeImpl(
    viskores::internal::make_LclCellShapeTag(shape), field, wCoords, pcoords, result);
}

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType&,
  const WorldCoordType&,
  const viskores::Vec<ParametricCoordType, 3>&,
  viskores::CellShapeTagEmpty,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  result = { 0 };
  return viskores::ErrorCode::OperationOnEmptyCell;
}

template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagPolyLine,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  viskores::IdComponent numPoints = field.GetNumberOfComponents();
  if (numPoints != wCoords.GetNumberOfComponents())
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  switch (numPoints)
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, viskores::CellShapeTagVertex(), result);
    case 2:
      return CellDerivative(field, wCoords, pcoords, viskores::CellShapeTagLine(), result);
  }

  auto dt = static_cast<ParametricCoordType>(1) / static_cast<ParametricCoordType>(numPoints - 1);
  auto idx = static_cast<viskores::IdComponent>(viskores::Ceil(pcoords[0] / dt));
  if (idx == 0)
  {
    idx = 1;
  }
  if (idx > numPoints - 1)
  {
    idx = numPoints - 1;
  }

  auto lineField = viskores::make_Vec(field[idx - 1], field[idx]);
  auto lineWCoords = viskores::make_Vec(wCoords[idx - 1], wCoords[idx]);
  auto pc = (pcoords[0] - static_cast<ParametricCoordType>(idx) * dt) / dt;
  return internal::CellDerivativeImpl(lcl::Line{}, lineField, lineWCoords, &pc, result);
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& field,
  const WorldCoordType& wCoords,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagPolygon,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  const viskores::IdComponent numPoints = field.GetNumberOfComponents();
  if ((numPoints <= 0) || (numPoints != wCoords.GetNumberOfComponents()))
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  switch (field.GetNumberOfComponents())
  {
    case 1:
      return CellDerivative(field, wCoords, pcoords, viskores::CellShapeTagVertex(), result);
    case 2:
      return CellDerivative(field, wCoords, pcoords, viskores::CellShapeTagLine(), result);
    default:
      return internal::CellDerivativeImpl(lcl::Polygon(numPoints), field, wCoords, pcoords, result);
  }
}

//-----------------------------------------------------------------------------
template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& field,
  const viskores::VecAxisAlignedPointCoordinates<2>& wCoords,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagQuad,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  return internal::CellDerivativeImpl(lcl::Pixel{}, field, wCoords, pcoords, result);
}

template <typename FieldVecType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& field,
  const viskores::VecAxisAlignedPointCoordinates<3>& wCoords,
  const viskores::Vec<ParametricCoordType, 3>& pcoords,
  viskores::CellShapeTagHexahedron,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  return internal::CellDerivativeImpl(lcl::Voxel{}, field, wCoords, pcoords, result);
}

//-----------------------------------------------------------------------------
/// \brief Take the derivative (get the gradient) of a point field in a cell.
///
/// Given the point field values for each node and the parametric coordinates
/// of a point within the cell, finds the derivative with respect to each
/// coordinate (i.e. the gradient) at that point. The derivative is not always
/// constant in some "linear" cells.
///
/// @param[in]  pointFieldValues A list of field values for each point in the cell. This
///     usually comes from a `FieldInPoint` argument in a
///     `viskores::worklet::WorkletVisitCellsWithPoints`.
/// @param[in]  worldCoordinateValues A list of world coordinates for each point in the cell. This
///     usually comes from a `FieldInPoint` argument in a
///     `viskores::worklet::WorkletVisitCellsWithPoints` where the coordinate system is passed
///     into that argument.
/// @param[in]  parametricCoords The parametric coordinates where you want to find the derivative.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] result Value to store the derivative/gradient. Because the derivative is taken
///     partially in the x, y, and z directions, the result is a `viskores::Vec` of size 3 with the
///     component type the same as the field. If the field is itself a vector, you get a `Vec`
///     of `Vec`s.
template <typename FieldVecType, typename WorldCoordType, typename ParametricCoordType>
VISKORES_EXEC viskores::ErrorCode CellDerivative(
  const FieldVecType& pointFieldValues,
  const WorldCoordType& worldCoordinateValues,
  const viskores::Vec<ParametricCoordType, 3>& parametricCoords,
  viskores::CellShapeTagGeneric shape,
  viskores::Vec<typename FieldVecType::ComponentType, 3>& result)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(
      status = CellDerivative(
        pointFieldValues, worldCoordinateValues, parametricCoords, CellShapeTag(), result));
    default:
      result = { 0 };
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

}
} // namespace viskores::exec

#endif //viskores_exec_Derivative_h
