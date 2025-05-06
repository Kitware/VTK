//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Line_h
#define lcl_Line_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Line : public Cell
{
public:
  constexpr LCL_EXEC Line() : Cell(ShapeId::LINE, 2) {}
  constexpr LCL_EXEC explicit Line(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Line tag) noexcept
{
  if (tag.shape() != ShapeId::LINE)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 2)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Line, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 0) = 0.5f;
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Line, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (pointId)
  {
    case 0:
      component(pcoords, 0) = 0.0f;
      return ErrorCode::SUCCESS;
    case 1:
      component(pcoords, 0) = 1.0f;
      return ErrorCode::SUCCESS;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Line, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);
  return internal::findParametricDistance(pcoords, 1);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Line, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  return component(pcoords, 0) >= T{0} && component(pcoords, 0) <= T{1};
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Line,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;
  using ResultCompType = ComponentType<Result>;

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto ival = internal::lerp(static_cast<ProcessingType>(values.getValue(0, c)),
                               static_cast<ProcessingType>(values.getValue(1, c)),
                               static_cast<ProcessingType>(component(pcoords, 0)));
    component(result, c) = static_cast<ResultCompType>(ival);
  }

  return ErrorCode::SUCCESS;
}

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Line,
  const Points& points,
  const Values& values,
  const CoordType&,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;
  using ResultCompType = ComponentType<Result>;

  ProcessingType dPt[3] = {
    static_cast<ProcessingType>(points.getValue(1, 0) - points.getValue(0, 0)),
    static_cast<ProcessingType>(points.getValue(1, 1) - points.getValue(0, 1)),
    static_cast<ProcessingType>(points.getValue(1, 2) - points.getValue(0, 2)) };

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto dv = static_cast<ProcessingType>(values.getValue(1, c) - values.getValue(0, c));
    component(dx, c) = (dPt[0] != 0.0f) ? static_cast<ResultCompType>(dv/dPt[0]) : ResultCompType{0};
    component(dy, c) = (dPt[1] != 0.0f) ? static_cast<ResultCompType>(dv/dPt[1]) : ResultCompType{0};
    component(dz, c) = (dPt[2] != 0.0f) ? static_cast<ResultCompType>(dv/dPt[2]) : ResultCompType{0};
  }
  return ErrorCode::SUCCESS;
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Line,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Line{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Line,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using T = ComponentType<PCoordType>;
  internal::Vector<T, 3> p0(static_cast<T>(points.getValue(0, 0)),
                            static_cast<T>(points.getValue(0, 1)),
                            static_cast<T>(points.getValue(0, 2)));
  internal::Vector<T, 3> p1(static_cast<T>(points.getValue(1, 0)),
                            static_cast<T>(points.getValue(1, 1)),
                            static_cast<T>(points.getValue(1, 2)));
  internal::Vector<T, 3> wc(static_cast<T>(component(wcoords, 0)),
                            static_cast<T>(component(wcoords, 1)),
                            static_cast<T>(component(wcoords, 2)));
  auto v1 = p1 - p0;
  auto v2 = wc - p0;
  component(pcoords, 0) = internal::dot(v1, v2) / internal::dot(v1, v1);
  return ErrorCode::SUCCESS;
}

} //namespace lcl

#endif //lcl_Line_h
