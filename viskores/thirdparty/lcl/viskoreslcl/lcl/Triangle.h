//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Triangle_h
#define lcl_Triangle_h

#include <lcl/ErrorCode.h>
#include <lcl/Shapes.h>

#include <lcl/internal/Common.h>

namespace lcl
{

class Triangle : public Cell
{
public:
  constexpr LCL_EXEC Triangle() : Cell(ShapeId::TRIANGLE, 3) {}
  constexpr LCL_EXEC explicit Triangle(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Triangle tag) noexcept
{
  if (tag.shape() != ShapeId::TRIANGLE)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() != 3)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Triangle, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  component(pcoords, 0) = T(1)/T(3);
  component(pcoords, 1) = T(1)/T(3);
  component(pcoords, 2) = T(0);
  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Triangle, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  component(pcoords, 2) = 0.0f;
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
      component(pcoords, 0) = 0.0f;
      component(pcoords, 1) = 1.0f;
      break;
    default:
      return ErrorCode::INVALID_POINT_ID;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Triangle, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  ComponentType<CoordType> weights[3];
  weights[0] = ComponentType<CoordType>{1} - component(pcoords, 0) - component(pcoords, 1);
  weights[1] = component(pcoords, 0);
  weights[2] = component(pcoords, 1);
  return internal::findParametricDistance(weights, 3);
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Triangle, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  return component(pcoords, 0) >= T{0} && component(pcoords, 1) >= T{0} &&
         (component(pcoords, 0) + component(pcoords, 1)) <= T{1};
}

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode interpolate(
  Triangle,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = internal::ClosestFloatType<typename Values::ValueType>;

  auto w0 = T(1) - static_cast<T>(component(pcoords, 0) + component(pcoords, 1));
  auto w1 = static_cast<T>(component(pcoords, 0));
  auto w2 = static_cast<T>(component(pcoords, 1));

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    auto v = static_cast<T>(values.getValue(0, c)) * w0 +
             static_cast<T>(values.getValue(1, c)) * w1 +
             static_cast<T>(values.getValue(2, c)) * w2;
    component(result, c) = static_cast<ComponentType<Result>>(v);
  }
  return ErrorCode::SUCCESS;
}

namespace internal
{

template <typename Values, typename CoordType, typename Result>
LCL_EXEC inline void parametricDerivative(
  Triangle, const Values& values, IdComponent comp, const CoordType&, Result&& result) noexcept
{
  component(result, 0) = static_cast<ComponentType<Result>>(values.getValue(1, comp) -
                                                            values.getValue(0, comp));
  component(result, 1) = static_cast<ComponentType<Result>>(values.getValue(2, comp) -
                                                            values.getValue(0, comp));
}

} // internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Triangle,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  return internal::derivative2D(Triangle{},
                                points,
                                values,
                                pcoords,
                                std::forward<Result>(dx),
                                std::forward<Result>(dy),
                                std::forward<Result>(dz));
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Triangle,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(Triangle{}, points, pcoords, std::forward<WCoordType>(wcoords));
}

//-----------------------------------------------------------------------------
// The following implementation is lifted from Viskores
//-----------------------------------------------------------------------------
// We will solve the world to parametric coordinates problem geometrically.
// Consider the parallelogram formed by wcoords and p0 of the triangle and
// the two adjacent edges. This parallelogram is equivalent to the
// axis-aligned rectangle anchored at the origin of parametric space.
//
//   p2 |\                 (1,0) |\                                        //
//      | \                      |  \                                      //
//      |  \                     |    \                                    //
//     |    \                    |      \                                  //
//     |     \                   |        \                                //
//     |      \                  |    (u,v) \                              //
//    | ---    \                 |-------*    \                            //
//    |    ---*wcoords           |       |      \                          //
//    |       |  \               |       |        \                        //
// p0 *---    |   \        (0,0) *------------------\ (1,0)                //
//        ---|     \                                                       //
//           x--    \                                                      //
//              ---  \                                                     //
//                 ---\ p1                                                 //
//
// In this diagram, the distance between p0 and the point marked x divided by
// the length of the edge it is on is equal, by proportionality, to the u
// parametric coordinate. (The v coordinate follows the other edge
// accordingly.) Thus, if we can find the intersection at x (or more
// specifically the distance between p0 and x), then we can find that
// parametric coordinate.
//
// Because the triangle is in 3-space, we are actually going to intersect the
// edge with a plane that is parallel to the opposite edge of p0 and
// perpendicular to the triangle. This is partially because it is easy to
// find the intersection between a plane and a line and partially because the
// computation will work for points not on the plane. (The result is
// equivalent to a point projected on the plane.)
//
// First, we define an implicit plane as:
//
// Dot((p - wcoords), planeNormal) = 0
//
// where planeNormal is the normal to the plane (easily computed from the
// triangle), and p is any point in the plane. Next, we define the parametric
// form of the line:
//
// p(d) = (p1 - p0)d + p0
//
// Where d is the fraction of distance from p0 toward p1. Note that d is
// actually equal to the parametric coordinate we are trying to find. Once we
// compute it, we are done. We can skip the part about finding the actual
// coordinates of the intersection.
//
// Solving for the intersection is as simple as substituting the line's
// definition of p(d) into p for the plane equation. With some basic algebra
// you get:
//
// d = Dot((wcoords - p0), planeNormal)/Dot((p1-p0), planeNormal)
//
// From here, the u coordinate is simply d. The v coordinate follows
// similarly.
//
template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Triangle,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  using TIn = typename Points::ValueType;
  using TOut = ComponentType<PCoordType>;

  internal::Vector<TIn, 3> pts[3];
  for (int i = 0; i < 3; ++i)
  {
    points.getTuple(i, pts[i]);
  }

  internal::Vector<TIn, 3> wc(component(wcoords, 0), component(wcoords, 1), component(wcoords, 2));

  auto triangleNormal = internal::cross(pts[1] - pts[0], pts[2] - pts[0]);
  for (IdComponent i = 0; i < 2; ++i)
  {
    auto& p0 = pts[0];
    auto& p1 = pts[i + 1];
    auto& p2 = pts[2 - i];
    auto planeNormal = internal::cross(triangleNormal, p2 - p0);

    component(pcoords, i) = static_cast<TOut>(internal::dot(wc - p0, planeNormal)) /
                            static_cast<TOut>(internal::dot(p1 - p0, planeNormal));
  }

  return ErrorCode::SUCCESS;
}

} //namespace lcl

#endif //lcl_Triangle_h
