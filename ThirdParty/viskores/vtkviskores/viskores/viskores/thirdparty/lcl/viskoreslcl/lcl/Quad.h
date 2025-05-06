//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Quad_h
#define lcl_Quad_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Quad : public Cell
{
public:
  constexpr LCL_EXEC Quad() : Cell(ShapeId::QUAD, 4) {}
  constexpr LCL_EXEC explicit Quad(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Quad tag) noexcept
{
  if (tag.shape() != ShapeId::QUAD && tag.shape() != ShapeId::PIXEL)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 4)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Quad, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 0) = 0.5f;
  component(pcoords, 1) = 0.5f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Quad, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (pointId)
  {
    case 0:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      break;
    case 1:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      break;
    case 2:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 1.0f;
      break;
    case 3:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      break;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Quad, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);
  return internal::findParametricDistance(pcoords, 2);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Quad, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;

  constexpr T eps = 0.001f;
  return component(pcoords, 0) >= -eps && component(pcoords, 0) <= (T{1} + eps) &&
         component(pcoords, 1) >= -eps && component(pcoords, 1) <= (T{1} + eps);
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Quad,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto v0 = internal::lerp(static_cast<T>(values.getValue(0, c)),
                             static_cast<T>(values.getValue(1, c)),
                             static_cast<T>(component(pcoords, 0)));
    auto v1 = internal::lerp(static_cast<T>(values.getValue(3, c)),
                             static_cast<T>(values.getValue(2, c)),
                             static_cast<T>(component(pcoords, 0)));
    auto v = internal::lerp(v0, v1, static_cast<T>(component(pcoords, 1)));
    component(result, c) = static_cast<ComponentType<Result>>(v);
  }

  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline void parametricDerivative(
  Quad, const Values& values, IdComponent comp, const CoordType& pcoords, Result&& result) noexcept
{
  using T = internal::ClosestFloatType<typename Values::ValueType>;
  T p0 = static_cast<T>(component(pcoords, 0));
  T p1 = static_cast<T>(component(pcoords, 1));
  T rm = T{1} - p0;
  T sm = T{1} - p1;

  T dr = (static_cast<T>(values.getValue(0, comp)) * -sm) +
         (static_cast<T>(values.getValue(1, comp)) *  sm) +
         (static_cast<T>(values.getValue(2, comp)) *  p1) +
         (static_cast<T>(values.getValue(3, comp)) * -p1);
  T ds = (static_cast<T>(values.getValue(0, comp)) * -rm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0) +
         (static_cast<T>(values.getValue(2, comp)) *  p0) +
         (static_cast<T>(values.getValue(3, comp)) *  rm);

  component(result, 0) = static_cast<ComponentType<Result>>(dr);
  component(result, 1) = static_cast<ComponentType<Result>>(ds);
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Quad,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  return internal::derivative2D(Quad{},
                                points,
                                values,
                                pcoords,
                                std::forward<Result>(dx),
                                std::forward<Result>(dy),
                                std::forward<Result>(dz));
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Quad,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Quad{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Quad,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  return internal::worldToParametric2D(Quad{}, points, wcoords, std::forward<PCoordType>(pcoords));
}

} //namespace lcl

#endif //lcl_Quad_h
