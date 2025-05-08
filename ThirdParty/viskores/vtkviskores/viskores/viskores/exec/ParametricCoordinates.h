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
#ifndef viskores_exec_ParametricCoordinates_h
#define viskores_exec_ParametricCoordinates_h

#include <viskores/Assert.h>
#include <viskores/CellShape.h>
#include <viskores/VecAxisAlignedPointCoordinates.h>
#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/FunctorBase.h>
#include <viskores/exec/internal/FastVec.h>
#include <viskores/internal/Assume.h>

#include <lcl/lcl.h>

namespace viskores
{
namespace exec
{

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  CellShapeTag,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  auto lclTag = typename viskores::internal::CellShapeTagViskoresToVtkc<CellShapeTag>::Type{};

  pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  if (numPoints != lclTag.numberOfPoints())
  {
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  return viskores::internal::LclErrorToViskoresError(lcl::parametricCenter(lclTag, pcoords));
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagEmpty,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  if (numPoints != 0)
  {
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  return viskores::ErrorCode::Success;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagVertex,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  if (numPoints != 1)
  {
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  return viskores::ErrorCode::Success;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagPolyLine,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesCenter(numPoints, viskores::CellShapeTagVertex(), pcoords);
    case 2:
      return ParametricCoordinatesCenter(numPoints, viskores::CellShapeTagLine(), pcoords);
  }
  pcoords[0] = 0.5;
  pcoords[1] = 0;
  pcoords[2] = 0;
  return viskores::ErrorCode::Success;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagPolygon,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  if (numPoints < 1)
  {
    pcoords = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesCenter(numPoints, viskores::CellShapeTagVertex(), pcoords);
    case 2:
      return ParametricCoordinatesCenter(numPoints, viskores::CellShapeTagLine(), pcoords);
    default:
      pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      return viskores::internal::LclErrorToViskoresError(
        lcl::parametricCenter(lcl::Polygon(numPoints), pcoords));
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric center of the given cell shape with the given number
/// of points.
///
/// @param[in]  numPoints The number of points in the cell.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] pcoords `viskores::Vec` to store the parametric center.
template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesCenter(
  viskores::IdComponent numPoints,
  viskores::CellShapeTagGeneric shape,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(
      status = ParametricCoordinatesCenter(numPoints, CellShapeTag(), pcoords));
    default:
      pcoords = { 0 };
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

//-----------------------------------------------------------------------------
template <typename ParametricCoordType, typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  CellShapeTag,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  auto lclTag = typename viskores::internal::CellShapeTagViskoresToVtkc<CellShapeTag>::Type{};

  if (numPoints != lclTag.numberOfPoints())
  {
    pcoords = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  if ((pointIndex < 0) || (pointIndex >= numPoints))
  {
    pcoords = { 0 };
    return viskores::ErrorCode::InvalidPointId;
  }

  pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  return viskores::internal::LclErrorToViskoresError(
    lcl::parametricPoint(lclTag, pointIndex, pcoords));
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent,
  viskores::IdComponent,
  viskores::CellShapeTagEmpty,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0;
  return viskores::ErrorCode::OperationOnEmptyCell;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::CellShapeTagVertex,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
  if (numPoints != 1)
  {
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  if (pointIndex != 0)
  {
    return viskores::ErrorCode::InvalidPointId;
  }
  return viskores::ErrorCode::Success;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::CellShapeTagPolyLine,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  if (numPoints < 1)
  {
    pcoords = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesPoint(
        numPoints, pointIndex, viskores::CellShapeTagVertex(), pcoords);
    case 2:
      return ParametricCoordinatesPoint(
        numPoints, pointIndex, viskores::CellShapeTagLine(), pcoords);
  }
  pcoords[0] =
    static_cast<ParametricCoordType>(pointIndex) / static_cast<ParametricCoordType>(numPoints - 1);
  pcoords[1] = 0;
  pcoords[2] = 0;
  return viskores::ErrorCode::Success;
}

template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::CellShapeTagPolygon,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesPoint(
        numPoints, pointIndex, viskores::CellShapeTagVertex(), pcoords);
    case 2:
      return ParametricCoordinatesPoint(
        numPoints, pointIndex, viskores::CellShapeTagLine(), pcoords);
    default:
      pcoords = viskores::TypeTraits<viskores::Vec<ParametricCoordType, 3>>::ZeroInitialization();
      return viskores::internal::LclErrorToViskoresError(
        lcl::parametricPoint(lcl::Polygon(numPoints), pointIndex, pcoords));
  }
}

//-----------------------------------------------------------------------------
/// Returns the parametric coordinate of a cell point of the given shape with
/// the given number of points.
///
/// @param[in]  numPoints The number of points in the cell.
/// @param[in]  pointIndex The local index for the point to get the parametric coordinates
///     of. This index is between 0 and _n_-1 where _n_ is the number of points in the cell.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] pcoords `viskores::Vec` to store the parametric center.
template <typename ParametricCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesPoint(
  viskores::IdComponent numPoints,
  viskores::IdComponent pointIndex,
  viskores::CellShapeTagGeneric shape,
  viskores::Vec<ParametricCoordType, 3>& pcoords)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(
      status = ParametricCoordinatesPoint(numPoints, pointIndex, CellShapeTag(), pcoords));
    default:
      pcoords[0] = pcoords[1] = pcoords[2] = 0;
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename LclCellShapeTag, typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinatesImpl(
  LclCellShapeTag tag,
  const WorldCoordVector& pointWCoords,
  const PCoordType& pcoords,
  typename WorldCoordVector::ComponentType& wcoords)
{
  return viskores::internal::LclErrorToViskoresError(lcl::parametricToWorld(
    tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), pcoords, wcoords));
}

} // namespace internal

template <typename WorldCoordVector, typename PCoordType, typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const WorldCoordVector& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  CellShapeTag shape,
  typename WorldCoordVector::ComponentType& result)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    viskores::internal::make_LclCellShapeTag(shape, numPoints), pointWCoords, pcoords, result);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const WorldCoordVector& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagEmpty empty,
  typename WorldCoordVector::ComponentType& result)
{
  return viskores::exec::CellInterpolate(pointWCoords, pcoords, empty, result);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const WorldCoordVector& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagPolyLine polyLine,
  typename WorldCoordVector::ComponentType& result)
{
  return viskores::exec::CellInterpolate(pointWCoords, pcoords, polyLine, result);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const WorldCoordVector& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagPolygon,
  typename WorldCoordVector::ComponentType& result)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, viskores::CellShapeTagVertex{}, result);
    case 2:
      return ParametricCoordinatesToWorldCoordinates(
        pointWCoords, pcoords, viskores::CellShapeTagLine{}, result);
    default:
      return internal::ParametricCoordinatesToWorldCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, pcoords, result);
  }
}

template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const viskores::VecAxisAlignedPointCoordinates<2>& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagQuad,
  typename WorldCoordVector::ComponentType& result)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, pcoords, result);
}

template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const viskores::VecAxisAlignedPointCoordinates<3>& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagHexahedron,
  typename WorldCoordVector::ComponentType& result)
{
  return internal::ParametricCoordinatesToWorldCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, pcoords, result);
}

//-----------------------------------------------------------------------------
/// Converts parametric coordinates (coordinates relative to the cell) to world coordinates
/// (coordinates in the global system).
///
/// @param[in]  pointWCoords A list of world coordinates for each point in the cell. This
///     usually comes from a `FieldInPoint` argument in a
///     `viskores::worklet::WorkletVisitCellsWithPoints` where the coordinate system is passed
///     into that argument.
/// @param[in]  pcoords The parametric coordinates where you want to get world coordinates for.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] result `viskores::Vec` to store the interpolated world coordinates.
template <typename WorldCoordVector, typename PCoordType>
static inline VISKORES_EXEC viskores::ErrorCode ParametricCoordinatesToWorldCoordinates(
  const WorldCoordVector& pointWCoords,
  const viskores::Vec<PCoordType, 3>& pcoords,
  viskores::CellShapeTagGeneric shape,
  typename WorldCoordVector::ComponentType& result)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(status = ParametricCoordinatesToWorldCoordinates(
                                    pointWCoords, pcoords, CellShapeTag(), result));
    default:
      result = { 0 };
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

//-----------------------------------------------------------------------------
namespace internal
{

template <typename LclCellShapeTag, typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinatesImpl(
  LclCellShapeTag tag,
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords,
  typename WorldCoordVector::ComponentType& result)
{
  if (pointWCoords.GetNumberOfComponents() != tag.numberOfPoints())
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  result = viskores::TypeTraits<typename WorldCoordVector::ComponentType>::ZeroInitialization();
  return viskores::internal::LclErrorToViskoresError(
    lcl::worldToParametric(tag, lcl::makeFieldAccessorNestedSOA(pointWCoords, 3), wcoords, result));
}

} // namespace internal

template <typename WorldCoordVector, typename CellShapeTag>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords,
  CellShapeTag shape,
  typename WorldCoordVector::ComponentType& result)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    viskores::internal::make_LclCellShapeTag(shape, numPoints), pointWCoords, wcoords, result);
}

template <typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector&,
  const typename WorldCoordVector::ComponentType&,
  viskores::CellShapeTagEmpty,
  typename WorldCoordVector::ComponentType& result)
{
  result = { 0 };
  return viskores::ErrorCode::OperationOnEmptyCell;
}

template <typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType&,
  viskores::CellShapeTagVertex,
  typename WorldCoordVector::ComponentType& result)
{
  if (pointWCoords.GetNumberOfComponents() != 1)
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }
  result = typename WorldCoordVector::ComponentType(0, 0, 0);
  return viskores::ErrorCode::Success;
}

template <typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords,
  viskores::CellShapeTagPolyLine,
  typename WorldCoordVector::ComponentType& result)
{
  viskores::IdComponent numPoints = pointWCoords.GetNumberOfComponents();
  if (numPoints < 1)
  {
    result = { 0 };
    return viskores::ErrorCode::InvalidNumberOfPoints;
  }

  if (numPoints == 1)
  {
    return WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, viskores::CellShapeTagVertex(), result);
  }

  using Vector3 = typename WorldCoordVector::ComponentType;
  using T = typename Vector3::ComponentType;

  //Find the closest vertex to the point.
  viskores::IdComponent idx = 0;
  Vector3 vec = pointWCoords[0] - wcoords;
  T minDistSq = viskores::Dot(vec, vec);
  for (viskores::IdComponent i = 1; i < numPoints; i++)
  {
    vec = pointWCoords[i] - wcoords;
    T d = viskores::Dot(vec, vec);

    if (d < minDistSq)
    {
      idx = i;
      minDistSq = d;
    }
  }

  //Find the right segment, and the parameterization along that segment.
  //Closest to 0, so segment is (0,1)
  if (idx == 0)
  {
    idx = 1;
  }

  viskores::Vec<Vector3, 2> line(pointWCoords[idx - 1], pointWCoords[idx]);
  Vector3 lpc;
  VISKORES_RETURN_ON_ERROR(
    WorldCoordinatesToParametricCoordinates(line, wcoords, viskores::CellShapeTagLine{}, lpc));

  //Segment param is [0,1] on that segment.
  //Map that onto the param for the entire segment.
  T dParam = static_cast<T>(1) / static_cast<T>(numPoints - 1);
  T polyLineParam = static_cast<T>(idx - 1) * dParam + lpc[0] * dParam;

  result = Vector3(polyLineParam, 0, 0);
  return viskores::ErrorCode::Success;
}

template <typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords,
  viskores::CellShapeTagPolygon,
  typename WorldCoordVector::ComponentType& result)
{
  auto numPoints = pointWCoords.GetNumberOfComponents();
  switch (numPoints)
  {
    case 1:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, viskores::CellShapeTagVertex{}, result);
    case 2:
      return WorldCoordinatesToParametricCoordinates(
        pointWCoords, wcoords, viskores::CellShapeTagLine{}, result);
    default:
      return internal::WorldCoordinatesToParametricCoordinatesImpl(
        lcl::Polygon(numPoints), pointWCoords, wcoords, result);
  }
}

static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const viskores::VecAxisAlignedPointCoordinates<2>& pointWCoords,
  const viskores::Vec3f& wcoords,
  viskores::CellShapeTagQuad,
  viskores::Vec3f& result)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Pixel{}, pointWCoords, wcoords, result);
}

static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const viskores::VecAxisAlignedPointCoordinates<3>& pointWCoords,
  const viskores::Vec3f& wcoords,
  viskores::CellShapeTagHexahedron,
  viskores::Vec3f& result)
{
  return internal::WorldCoordinatesToParametricCoordinatesImpl(
    lcl::Voxel{}, pointWCoords, wcoords, result);
}

//-----------------------------------------------------------------------------
/// Converts world coordinates (coordinates in the global system) to parametric
/// coordinates (coordinates relative to the cell). This function can be slow for
/// cell types with nonlinear interpolation (which is anything that is not a simplex).
///
/// @param[in]  pointWCoords A list of world coordinates for each point in the cell. This
///     usually comes from a `FieldInPoint` argument in a
///     `viskores::worklet::WorkletVisitCellsWithPoints` where the coordinate system is passed
///     into that argument.
/// @param[in]  wcoords The world coordinates where you want to get parametric coordinates for.
/// @param[in]  shape A tag of type `CellShapeTag*` to identify the shape of the cell.
///     This method is overloaded for different shape types.
/// @param[out] result `viskores::Vec` to store the associated parametric coordinates.
template <typename WorldCoordVector>
static inline VISKORES_EXEC viskores::ErrorCode WorldCoordinatesToParametricCoordinates(
  const WorldCoordVector& pointWCoords,
  const typename WorldCoordVector::ComponentType& wcoords,
  viskores::CellShapeTagGeneric shape,
  typename WorldCoordVector::ComponentType& result)
{
  viskores::ErrorCode status;
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(status = WorldCoordinatesToParametricCoordinates(
                                    pointWCoords, wcoords, CellShapeTag(), result));
    default:
      result = { 0 };
      status = viskores::ErrorCode::InvalidShapeId;
  }
  return status;
}

}
} // namespace viskores::exec

#endif //viskores_exec_ParametricCoordinates_h
