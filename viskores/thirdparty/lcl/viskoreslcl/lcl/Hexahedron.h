//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Hexahedron_h
#define lcl_Hexahedron_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Hexahedron : public Cell
{
public:
  constexpr LCL_EXEC Hexahedron() : Cell(ShapeId::HEXAHEDRON, 8) {}
  constexpr LCL_EXEC explicit Hexahedron(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Hexahedron tag) noexcept
{
  if (tag.shape() != ShapeId::HEXAHEDRON && tag.shape() != ShapeId::VOXEL)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 8)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Hexahedron, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 0) = 0.5f;
  component(pcoords, 1) = 0.5f;
  component(pcoords, 2) = 0.5f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Hexahedron, IdComponent pointId, CoordType&& pcoords) noexcept
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
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 3:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 0.0f;
      break;
    case 4:
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 1.0f;
      break;
    case 5:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 0.0f;
      component(pcoords, 2) = 1.0f;
      break;
    case 6:
      component(pcoords, 0) = 1.0f;
      component(pcoords, 1) = 1.0f;
      component(pcoords, 2) = 1.0f;
      break;
    case 7:
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
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Hexahedron, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);
  return internal::findParametricDistance(pcoords, 3);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Hexahedron, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;

  constexpr T eps = 1e-6f;
  return component(pcoords, 0) >= -eps && component(pcoords, 0) <= (T{1} + eps) &&
         component(pcoords, 1) >= -eps && component(pcoords, 1) <= (T{1} + eps) &&
         component(pcoords, 2) >= -eps && component(pcoords, 2) <= (T{1} + eps);
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Hexahedron,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto vbf = internal::lerp(static_cast<T>(values.getValue(0, c)),
                              static_cast<T>(values.getValue(1, c)),
                              static_cast<T>(component(pcoords, 0)));
    auto vbb = internal::lerp(static_cast<T>(values.getValue(3, c)),
                              static_cast<T>(values.getValue(2, c)),
                              static_cast<T>(component(pcoords, 0)));
    auto vtf = internal::lerp(static_cast<T>(values.getValue(4, c)),
                              static_cast<T>(values.getValue(5, c)),
                              static_cast<T>(component(pcoords, 0)));
    auto vtb = internal::lerp(static_cast<T>(values.getValue(7, c)),
                              static_cast<T>(values.getValue(6, c)),
                              static_cast<T>(component(pcoords, 0)));
    auto vb = internal::lerp(vbf, vbb, static_cast<T>(component(pcoords, 1)));
    auto vt = internal::lerp(vtf, vtb, static_cast<T>(component(pcoords, 1)));
    auto v = internal::lerp(vb, vt, static_cast<T>(component(pcoords, 2)));
    component(result, c) = static_cast<ComponentType<Result>>(v);
  }

  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline void parametricDerivative(Hexahedron,
                                           const Values& values,
                                           IdComponent comp,
                                           const CoordType& pcoords,
                                           Result&& result) noexcept
{
  using T = internal::ClosestFloatType<typename Values::ValueType>;
  T p0 = static_cast<T>(component(pcoords, 0));
  T p1 = static_cast<T>(component(pcoords, 1));
  T p2 = static_cast<T>(component(pcoords, 2));
  T rm = T{1} - p0;
  T sm = T{1} - p1;
  T tm = T{1} - p2;

  T dr = (static_cast<T>(values.getValue(0, comp)) * -sm * tm) +
         (static_cast<T>(values.getValue(1, comp)) *  sm * tm) +
         (static_cast<T>(values.getValue(2, comp)) *  p1 * tm) +
         (static_cast<T>(values.getValue(3, comp)) * -p1 * tm) +
         (static_cast<T>(values.getValue(4, comp)) * -sm * p2) +
         (static_cast<T>(values.getValue(5, comp)) *  sm * p2) +
         (static_cast<T>(values.getValue(6, comp)) *  p1 * p2) +
         (static_cast<T>(values.getValue(7, comp)) * -p1 * p2);

  T ds = (static_cast<T>(values.getValue(0, comp)) * -rm * tm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0 * tm) +
         (static_cast<T>(values.getValue(2, comp)) *  p0 * tm) +
         (static_cast<T>(values.getValue(3, comp)) *  rm * tm) +
         (static_cast<T>(values.getValue(4, comp)) * -rm * p2) +
         (static_cast<T>(values.getValue(5, comp)) * -p0 * p2) +
         (static_cast<T>(values.getValue(6, comp)) *  p0 * p2) +
         (static_cast<T>(values.getValue(7, comp)) *  rm * p2);

  T dt = (static_cast<T>(values.getValue(0, comp)) * -rm * sm) +
         (static_cast<T>(values.getValue(1, comp)) * -p0 * sm) +
         (static_cast<T>(values.getValue(2, comp)) * -p0 * p1) +
         (static_cast<T>(values.getValue(3, comp)) * -rm * p1) +
         (static_cast<T>(values.getValue(4, comp)) *  rm * sm) +
         (static_cast<T>(values.getValue(5, comp)) *  p0 * sm) +
         (static_cast<T>(values.getValue(6, comp)) *  p0 * p1) +
         (static_cast<T>(values.getValue(7, comp)) *  rm * p1);

  component(result, 0) = static_cast<ComponentType<Result>>(dr);
  component(result, 1) = static_cast<ComponentType<Result>>(ds);
  component(result, 2) = static_cast<ComponentType<Result>>(dt);
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Hexahedron,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  return internal::derivative3D(Hexahedron{},
                                points,
                                values,
                                pcoords,
                                std::forward<Result>(dx),
                                std::forward<Result>(dy),
                                std::forward<Result>(dz));
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Hexahedron,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Hexahedron{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Hexahedron,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  return internal::worldToParametric3D(
    Hexahedron{}, points, wcoords, std::forward<PCoordType>(pcoords));
}

} // lcl

#endif // lcl_Hexahedron_h
