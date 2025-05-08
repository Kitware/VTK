//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_lcl_h
#define lcl_lcl_h

#include <lcl/Hexahedron.h>
#include <lcl/Line.h>
#include <lcl/Pixel.h>
#include <lcl/Polygon.h>
#include <lcl/Pyramid.h>
#include <lcl/Quad.h>
#include <lcl/Tetra.h>
#include <lcl/Triangle.h>
#include <lcl/Vertex.h>
#include <lcl/Voxel.h>
#include <lcl/Wedge.h>

#include <utility>

namespace lcl
{

/// \brief Perform basic checks to validate cell's state.
/// \param[in]  tag  The cell tag to validate.
/// \return          lcl::ErrorCode::SUCCESS if valid.
///
LCL_EXEC inline lcl::ErrorCode validate(Cell tag) noexcept
{
  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = validate(CellTag{tag}));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return center of a cell in parametric coordinates.
/// \remark Note that the parametric center is not always located at (0.5,0.5,0.5).
/// \param[in]   tag      The cell tag.
/// \param[out]  pcoords  The center of the cell in parametric coordinates.
/// \return               A lcl::ErrorCode value indicating the status of the operation.
///
template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Cell tag, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = parametricCenter(CellTag{tag}, std::forward<CoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return the parametric coordinates of a cell's point.
/// \param[in]   tag      The cell tag.
/// \param[in]   pointId  The point number.
/// \param[out]  pcoords  The parametric coordinates of a cell's point.
/// \return               A lcl::ErrorCode value indicating the status of the operation.
///
template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Cell tag, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = parametricPoint(CellTag{tag}, pointId, std::forward<CoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
      break;
  }
  return status;
}

/// \brief Return the parametric distance of a parametric coordinate to a cell.
/// \param[in]  tag      The cell tag.
/// \param[in]  pcoords  The parametric coordinates of the point.
/// \return              The parametric distance of the point to the cell.
///                      If point is inside the cell, 0 is returned.
/// \pre tag should be a valid cell, otherwise the result is undefined.
///
template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Cell tag, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ComponentType<CoordType> dist{0};
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(dist = parametricDistance(CellTag{tag}, pcoords));
    default:
      break;
  }
  return dist;
}

/// \brief Check if the given parametric point lies inside a cell.
/// \param[in]  tag      The cell tag.
/// \param[in]  pcoords  The parametric coordinates of the point.
/// \return              true if inside, false otherwise.
/// \pre tag should be a valid cell, otherwise the result is undefined.
///
template<typename CoordType>
LCL_EXEC inline bool cellInside(Cell tag, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  bool inside = false;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(inside = cellInside(CellTag{tag}, pcoords));
    default:
      break;
  }
  return inside;
}

/// \brief Interpolate \c values at the paramteric coordinates \c pcoords
/// \param[in]   tag      The cell tag.
/// \param[in]   values   A \c FieldAccessor for values to interpolate
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  result   The interpolation result
/// \return               A lcl::ErrorCode value indicating the status of the operation.
///
template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Cell tag,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = interpolate(CellTag{tag}, values, pcoords, std::forward<Result>(result)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute derivative of \c values at the paramteric coordinates \c pcoords
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   values   A \c FieldAccessor for the values to compute derivative of.
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  dx       The derivative along X
/// \param[out]  dy       The derivative along Y
/// \param[out]  dz       The derivative along Z
/// \return               A lcl::ErrorCode value indicating the status of the operation.
///
template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Cell tag,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = derivative(CellTag{tag},
                                                points,
                                                values,
                                                pcoords,
                                                std::forward<Result>(dx),
                                                std::forward<Result>(dy),
                                                std::forward<Result>(dz)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute world coordinates from parametric coordinates
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   pcoords  The parametric coordinates.
/// \param[out]  wcoords  The world coordinates.
///
template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Cell tag,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = parametricToWorld(CellTag{tag}, points, pcoords, std::forward<WCoordType>(wcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

/// \brief Compute parametric coordinates from world coordinates
/// \param[in]   tag      The cell tag.
/// \param[in]   points   A \c FieldAccessor for points of the cell
/// \param[in]   wcoords  The world coordinates.
/// \param[out]  pcoords  The parametric coordinates.
///
template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Cell tag,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  ErrorCode status = ErrorCode::SUCCESS;
  switch (tag.shape())
  {
    lclGenericCellShapeMacro(status = worldToParametric(CellTag{tag}, points, wcoords, std::forward<PCoordType>(pcoords)));
    default:
      status = ErrorCode::INVALID_SHAPE_ID;
  }
  return status;
}

} //namespace lcl

#endif //lcl_lcl_h
