//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Wedge_h
#define lcl_Wedge_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Wedge : public Cell
{
public:
  constexpr LCL_EXEC Wedge() : Cell(ShapeId::WEDGE, 6) {}
  constexpr LCL_EXEC explicit Wedge(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Wedge tag) noexcept
{
  if (tag.shape() != ShapeId::WEDGE)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 6)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Wedge, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  component(pcoords, 0) = T(1)/T(3);
  component(pcoords, 1) = T(1)/T(3);
  component(pcoords, 2) = 0.5f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Wedge, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (pointId)
  {
    case 0:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 1:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 2:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 3:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 1.0f;
      break;
    case 4:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 1.0f;
      break;
    case 5:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 1.0f;
      break;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Wedge, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);
  return internal::findParametricDistance(pcoords, 3);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Wedge, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;

  constexpr T eps = 0.001f;
  return component(pcoords, 0) >= -eps &&
         component(pcoords, 1) >= -eps &&
         component(pcoords, 2) >= -eps &&
         (component(pcoords, 0) + component(pcoords, 1)) <= (T{1} + eps) &&
         component(pcoords, 2) <= (T{1} + eps);
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC lcl::ErrorCode interpolate(
  Wedge,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  auto p0 = static_cast<T>(component(pcoords, 0));
  auto p1 = static_cast<T>(component(pcoords, 1));
  auto p2 = static_cast<T>(component(pcoords, 2));
  auto sm = T{1} - p0 - p1;

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    T t1 = static_cast<T>(values.getValue(0, c)) * sm +
           static_cast<T>(values.getValue(1, c)) * p0 +
           static_cast<T>(values.getValue(2, c)) * p1;
    T t2 = static_cast<T>(values.getValue(3, c)) * sm +
           static_cast<T>(values.getValue(4, c)) * p0 +
           static_cast<T>(values.getValue(5, c)) * p1;
    component(result, c) = static_cast<ComponentType<Result>>(internal::lerp(t1, t2, p2));
  }

  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline void parametricDerivative(
  Wedge, const Values& values, IdComponent comp, const CoordType& pcoords, Result&& result) noexcept
{
  using T = internal::ClosestFloatType<typename Values::ValueType>;
  auto p0 = static_cast<T>(component(pcoords, 0));
  auto p1 = static_cast<T>(component(pcoords, 1));
  auto p2 = static_cast<T>(component(pcoords, 2));
  auto rm = T{1} - p2;
  auto sm = T{1} - p0 - p1;

  T dr = (static_cast<T>(values.getValue(0, comp)) * -rm) +
         (static_cast<T>(values.getValue(1, comp)) *  rm) +
         (static_cast<T>(values.getValue(3, comp)) * -p2) +
         (static_cast<T>(values.getValue(4, comp)) *  p2);

  T ds = (static_cast<T>(values.getValue(0, comp)) * -rm) +
         (static_cast<T>(values.getValue(2, comp)) *  rm) +
         (static_cast<T>(values.getValue(3, comp)) * -p2) +
         (static_cast<T>(values.getValue(5, comp)) *  p2);

  T dt = (static_cast<T>(values.getValue(0, comp)) * -sm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0) +
         (static_cast<T>(values.getValue(2, comp)) * -p1) +
         (static_cast<T>(values.getValue(3, comp)) *  sm) +
         (static_cast<T>(values.getValue(4, comp)) *  p0) +
         (static_cast<T>(values.getValue(5, comp)) *  p1);

  component(result, 0) = static_cast<ComponentType<Result>>(dr);
  component(result, 1) = static_cast<ComponentType<Result>>(ds);
  component(result, 2) = static_cast<ComponentType<Result>>(dt);
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Wedge,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  return internal::derivative3D(Wedge{},
                                points,
                                values,
                                pcoords,
                                std::forward<Result>(dx),
                                std::forward<Result>(dy),
                                std::forward<Result>(dz));
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Wedge,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Wedge{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Wedge,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  return internal::worldToParametric3D(Wedge{}, points, wcoords, std::forward<PCoordType>(pcoords));
}

} // lcl

#endif // lcl_Wedge_h
