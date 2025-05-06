//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef lcl_Polygon_h
#define lcl_Polygon_h

#include <lcl/ErrorCode.h>
#include <lcl/Quad.h>
#include <lcl/Shapes.h>
#include <lcl/Triangle.h>

#include <lcl/internal/Common.h>

namespace lcl
{

/// \c Polygon with 3 and 4 points behave exactly as \c Triangle and \c Quad
/// respectively. For 5 or more points, the points are arranged such that
/// they are on the circle circumscribed in the
/// unit square from 0 to 1. That is, the point are on the circle centered at
/// coordinate 0.5,0.5 with radius 0.5. The polygon is divided into regions
/// defined by the triangle fan formed by the points around the center. This
/// is C0 continuous but not necessarily C1 continuous. It is also possible to
/// have a non 1 to 1 mapping between parametric coordinates world coordinates
/// if the polygon is not planar or convex.
class Polygon : public Cell
{
public:
  constexpr LCL_EXEC Polygon() : Cell(ShapeId::POLYGON, 3) {}
  constexpr LCL_EXEC explicit Polygon(lcl::IdComponent numPoints)
    : Cell(ShapeId::POLYGON, numPoints)
  {
  }
  constexpr LCL_EXEC explicit Polygon(const Cell& cell) : Cell(cell) {}
};

LCL_EXEC inline lcl::ErrorCode validate(Polygon tag) noexcept
{
  if (tag.shape() != ShapeId::POLYGON)
  {
    return ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE;
  }
  if (tag.numberOfPoints() < 3)
  {
    return ErrorCode::INVALID_NUMBER_OF_POINTS;
  }

  return ErrorCode::SUCCESS;
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricCenter(Polygon tag, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (tag.numberOfPoints())
  {
    case 3:
      return parametricCenter(Triangle{}, pcoords);
    case 4:
      return parametricCenter(Quad{}, pcoords);
    default:
      component(pcoords, 0) = 0.5f;
      component(pcoords, 1) = 0.5f;
      return ErrorCode::SUCCESS;
  }
}

template<typename CoordType>
LCL_EXEC inline lcl::ErrorCode parametricPoint(
  Polygon tag, IdComponent pointId, CoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  if (pointId < 0 || pointId >= tag.numberOfPoints())
  {
    return ErrorCode::INVALID_POINT_ID;
  }

  switch (tag.numberOfPoints())
  {
    case 3:
      return parametricPoint(Triangle{}, pointId, pcoords);
    case 4:
      return parametricPoint(Quad{}, pointId, pcoords);
    default:
    {
      using T = ComponentType<CoordType>;
      constexpr double two_pi = 2.0 * 3.14159265359;
      auto angle = (static_cast<T>(pointId) * static_cast<T>(two_pi)) / static_cast<T>(tag.numberOfPoints());
      component(pcoords, 0) = 0.5f * (LCL_MATH_CALL(cos, (angle)) + 1.0f);
      component(pcoords, 1) = 0.5f * (LCL_MATH_CALL(sin, (angle)) + 1.0f);
      return ErrorCode::SUCCESS;
    }
  }
}

template<typename CoordType>
LCL_EXEC inline ComponentType<CoordType> parametricDistance(Polygon tag, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (tag.numberOfPoints())
  {
    case 3:
      return parametricDistance(Triangle{}, pcoords);
    default:
      return internal::findParametricDistance(pcoords, 2);
  }
}

template<typename CoordType>
LCL_EXEC inline bool cellInside(Polygon tag, const CoordType& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  using T = ComponentType<CoordType>;
  switch (tag.numberOfPoints())
  {
    case 3:
      return cellInside(Triangle{}, pcoords);
    case 4:
      return cellInside(Quad{}, pcoords);
    default:
      break;
  }

  constexpr T epsilon = std::is_same<T, float>::value ? T(1e-5f) : T(1e-9f);

  auto x = component(pcoords, 0) - T(0.5f);
  auto y = component(pcoords, 1) - T(0.5f);
  auto dist2 = (x * x) + (y * y);
  if (dist2 > 0.25f) // definitely outside
  {
    return false;
  }
  else if (LCL_MATH_CALL(abs, (x)) < (T(4) * epsilon) && LCL_MATH_CALL(abs, (y)) < (T(4) * epsilon))
  {
    return true; // at the center
  }
  else
  {
    constexpr double two_pi = 2.0 * 3.14159265359;
    T deltaAngle = static_cast<T>(two_pi) / static_cast<T>(tag.numberOfPoints());
    T apothem = 0.5f * LCL_MATH_CALL(cos, (deltaAngle/2.0f));
    if (dist2 <= (apothem * apothem)) // inside in-circle
    {
      return true;
    }

    // compute distance at which the line, from the center, through the given point, intersects
    // the polygon edge
    T angle = LCL_MATH_CALL(atan2, (y), (x));
    if (angle < T(0))
    {
      angle += static_cast<T>(two_pi);
    }

    T a2 = angle - (LCL_MATH_CALL(floor, (angle / deltaAngle)) * deltaAngle);
    T maxDist = apothem / LCL_MATH_CALL(cos, (LCL_MATH_CALL(abs, (deltaAngle/2.0f - a2))));
    return dist2 <= (maxDist * maxDist);
  }
}

namespace internal
{

template <typename CoordType>
LCL_EXEC inline lcl::ErrorCode polygonToSubTrianglePCoords(
  Polygon tag,
  const CoordType& polygonPC,
  IdComponent& p0,
  IdComponent& p1,
  ComponentType<CoordType> trianglePC[2]) noexcept
{
  using T = ComponentType<CoordType>;

  constexpr T epsilon = std::is_same<T, float>::value ? T(1e-5f) : T(1e-9f);

  // Find the sub-triangle containing pcoords
  auto x = component(polygonPC, 0) - T(0.5f);
  auto y = component(polygonPC, 1) - T(0.5f);
  if (LCL_MATH_CALL(abs, (x)) < (T(4) * epsilon) && LCL_MATH_CALL(abs, (y)) < (T(4) * epsilon))
  {
    // we are at the center
    p0 = 0;
    p1 = 1;
    trianglePC[0] = trianglePC[1] = T(0);
    return ErrorCode::SUCCESS;
  }

  constexpr double two_pi = 2.0 * 3.14159265359;
  T angle = LCL_MATH_CALL(atan2, (y), (x));
  if (angle < T(0))
  {
    angle += static_cast<T>(two_pi);
  }
  T deltaAngle = static_cast<T>(two_pi) / static_cast<T>(tag.numberOfPoints());

  p0 = static_cast<IdComponent>(LCL_MATH_CALL(floor, (angle / deltaAngle)));
  p1 = (p0 + 1) % tag.numberOfPoints();

  // Build triangle with polygon pcoords as its wcoords
  T triPts[9] = { T(0), T(0), T(0), T(0), T(0), T(0), T(0), T(0), T(0) };
  LCL_RETURN_ON_ERROR(parametricCenter(tag, triPts))
  LCL_RETURN_ON_ERROR(parametricPoint(tag, p0, triPts + 3))
  LCL_RETURN_ON_ERROR(parametricPoint(tag, p1, triPts + 6))

  // Find the parametric coord on the triangle
  T triWC[3] = { component(polygonPC, 0), component(polygonPC, 1), T(0) };
  LCL_RETURN_ON_ERROR(worldToParametric(Triangle{}, makeFieldAccessorFlatSOAConst(triPts, 3), triWC, trianglePC))

  return ErrorCode::SUCCESS;
}

template <typename Values>
LCL_EXEC inline typename Values::ValueType polygonInterpolateComponentAtCenter(
  Polygon tag, const Values& values, IdComponent comp) noexcept
{
  using T = internal::ClosestFloatType<typename Values::ValueType>;

  auto weight = T{1} / static_cast<T>(tag.numberOfPoints());
  auto result = static_cast<T>(values.getValue(0, comp));
  for (IdComponent i = 1; i < tag.numberOfPoints(); ++i)
  {
    result += static_cast<T>(values.getValue(i, comp));
  }
  result *= weight;

  return static_cast<typename Values::ValueType>(result);
}

} // namespace internal

template <typename Values, typename CoordType, typename Result>
LCL_EXEC lcl::ErrorCode interpolate(
  Polygon tag,
  const Values& values,
  const CoordType& pcoords,
  Result&& result) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (tag.numberOfPoints())
  {
    case 3:
      return interpolate(Triangle{}, values, pcoords, std::forward<Result>(result));
    case 4:
      return interpolate(Quad{}, values, pcoords, std::forward<Result>(result));
    default:
      break;
  }

  using ResultCompType = ComponentType<Result>;
  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;

  IdComponent p0, p1;
  ComponentType<CoordType> triPc[2];
  LCL_RETURN_ON_ERROR(internal::polygonToSubTrianglePCoords(tag, pcoords, p0, p1, triPc))

  // compute polygon interpolation from triangle weights
  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    ProcessingType triVals[3];
    triVals[0] = static_cast<ProcessingType>(internal::polygonInterpolateComponentAtCenter(tag, values, c));
    triVals[1] = static_cast<ProcessingType>(values.getValue(p0, c));
    triVals[2] = static_cast<ProcessingType>(values.getValue(p1, c));
    ResultCompType val = 0;
    LCL_RETURN_ON_ERROR(interpolate(Triangle{}, makeFieldAccessorNestedSOA(triVals), triPc, &val))
    component(result, c) = val;
  }

  return ErrorCode::SUCCESS;
}

namespace internal
{
// To find the gradient in a polygon (of 5 or more points), we will extract a small triangle near
// the desired parameteric coordinates (pcoords). We return the field values (outField) and world
// coordinates (outWCoords) for this triangle, which is all that is needed to find the gradient
// in a triangle.
//
// The trangle will be "pointing" away from the center of the polygon, and pcoords will be placed
// at the apex of the triangle. This is because if pcoords is at or near the edge of the polygon,
// we do not want to push any of the points over the edge, and it is not trivial to determine
// exactly where the edge of the polygon is.
template <typename CoordType>
LCL_EXEC inline void polygonGetTriangleAroundPCoords(
  const CoordType& pcoords, ComponentType<CoordType> pc1[2], ComponentType<CoordType> pc2[2]) noexcept
{
  using T = ComponentType<CoordType>;

  // Find the unit vector pointing from the center of the polygon to pcoords
  Vector<T, 2> radialVector(component(pcoords, 0) - 0.5f, component(pcoords, 1) - 0.5f);
  auto magSqr = dot(radialVector, radialVector);
  if (magSqr > 8.0f * 1e-4f)
  {
    radialVector /= LCL_MATH_CALL(sqrt, (magSqr));
  }
  else
  {
    // pcoords is in the center of the polygon. Just point in an arbitrary direction
    radialVector[0] = T(1);
    radialVector[1] = T(0);
  }

  // We want the two points away from pcoords to be back toward the center but moved at 45 degrees
  // off the radius. Simple geometry shows us that the (not quite unit) vectors of those two
  // directions are (-radialVector[1] - radialVector[0], radialVector[0] - radialVector[1]) and
  // (radialVector[1] - radialVector[0], -radialVector[0] - radialVector[1]).
  //
  //  *\ (-radialVector[1], radialVector[0])                                           //
  //  |  \                                                                             //
  //  |    \ (-radialVector[1] - radialVector[0], radialVector[0] - radialVector[1])   //
  //  |      \                                                                         //
  //  +-------* radialVector                                                           //
  //  |      /                                                                         //
  //  |    / (radialVector[1] - radialVector[0], -radialVector[0] - radialVector[1])   //
  //  |  /                                                                             //
  //  */ (radialVector[1], -radialVector[0])                                           //

  // This scaling value is somewhat arbitrary. It is small enough to be "close" to the selected
  // point and small enough to be guaranteed to be inside the polygon, but large enough to
  // get an accurate gradient.
  static constexpr T scale = 0.05f;

  pc1[0] = pcoords[0] + scale * (-radialVector[1] - radialVector[0]);
  pc1[1] = pcoords[1] + scale * (radialVector[0] - radialVector[1]);

  pc2[0] = pcoords[0] + scale * (radialVector[1] - radialVector[0]);
  pc2[1] = pcoords[1] + scale * (-radialVector[0] - radialVector[1]);
}

} // namespace internal

template <typename Points, typename Values, typename CoordType, typename Result>
LCL_EXEC inline lcl::ErrorCode derivative(
  Polygon tag,
  const Points& points,
  const Values& values,
  const CoordType& pcoords,
  Result&& dx,
  Result&& dy,
  Result&& dz) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(CoordType);

  switch (tag.numberOfPoints())
  {
    case 3:
      return derivative(Triangle{},
                        points,
                        values,
                        pcoords,
                        std::forward<Result>(dx),
                        std::forward<Result>(dy),
                        std::forward<Result>(dz));
    case 4:
      return derivative(Quad{},
                        points,
                        values,
                        pcoords,
                        std::forward<Result>(dx),
                        std::forward<Result>(dy),
                        std::forward<Result>(dz));
    default:
      break;
  }

  using ResultCompType = ComponentType<Result>;
  using ProcessingType = internal::ClosestFloatType<typename Values::ValueType>;

  // Get the parametric coordinates of a small triangle, with pcoords as one of the vertices
  ComponentType<CoordType> ptPc1[2], ptPc2[2];
  internal::polygonGetTriangleAroundPCoords(pcoords, ptPc1, ptPc2);

  // Compute world coordinates of the points of the triangle
  internal::Vector<ProcessingType, 3> triPts[3];
  LCL_RETURN_ON_ERROR(interpolate(tag, points, pcoords, triPts[0]))
  LCL_RETURN_ON_ERROR(interpolate(tag, points, ptPc1, triPts[1]))
  LCL_RETURN_ON_ERROR(interpolate(tag, points, ptPc2, triPts[2]))

  // Compute the derivative on the triangle
  //----------------------------------------
  // 2-D coordinate system on the triangle's plane
  internal::Space2D<ProcessingType> triSpace(triPts[0], triPts[1], triPts[2]);
  internal::Vector<ProcessingType, 2> pts2d[3];
  for (int i = 0; i < 3; ++i)
  {
    pts2d[i] = triSpace.to2DPoint(triPts[i]);
  }

  // pre-compute once
  internal::Matrix<ProcessingType, 2, 2> jacobian;
  internal::jacobian2D(Triangle{}, makeFieldAccessorNestedSOA(pts2d, 2), nullptr, jacobian);
  internal::Matrix<ProcessingType, 2, 2> invJacobian;
  LCL_RETURN_ON_ERROR(internal::matrixInverse(jacobian, invJacobian))

  // Compute sub-triangle information of the three vertices of the derivation triangle to
  // reduce the amount of redundant computations in the loop.
  IdComponent subP1P2[3][2];
  ComponentType<CoordType> pcs[3][2];
  internal::polygonToSubTrianglePCoords(tag, pcoords, subP1P2[0][0], subP1P2[0][1], pcs[0]);
  internal::polygonToSubTrianglePCoords(tag, ptPc1, subP1P2[1][0], subP1P2[1][1], pcs[1]);
  internal::polygonToSubTrianglePCoords(tag, ptPc2, subP1P2[2][0], subP1P2[2][1], pcs[2]);

  for (IdComponent c = 0; c < values.getNumberOfComponents(); ++c)
  {
    // Interpolate component values at the vertices of the derivation triangle.
    auto vCenter = static_cast<ProcessingType>(internal::polygonInterpolateComponentAtCenter(tag, values, c));
    ProcessingType triVals[3];
    for (int i = 0; i < 3; ++i)
    {
      ProcessingType field[3] = {vCenter,
                                 static_cast<ProcessingType>(values.getValue(subP1P2[i][0], c)),
                                 static_cast<ProcessingType>(values.getValue(subP1P2[i][1], c))};
      LCL_RETURN_ON_ERROR(interpolate(Triangle{}, makeFieldAccessorNestedSOA(field), pcs[i], triVals + i))
    }

    // Compute derivative in the triangle
    internal::Vector<ProcessingType, 2> dvdp;
    parametricDerivative(Triangle{}, makeFieldAccessorNestedSOA(triVals), 0, nullptr, dvdp);
    auto d2D = matrixMultiply(dvdp, invJacobian);
    auto d3D = triSpace.to3DVec(d2D);

    component(dx, c) = static_cast<ResultCompType>(d3D[0]);
    component(dy, c) = static_cast<ResultCompType>(d3D[1]);
    component(dz, c) = static_cast<ResultCompType>(d3D[2]);
  }

  return ErrorCode::SUCCESS;
}

template <typename Points, typename PCoordType, typename WCoordType>
LCL_EXEC inline lcl::ErrorCode parametricToWorld(
  Polygon tag,
  const Points& points,
  const PCoordType& pcoords,
  WCoordType&& wcoords) noexcept
{
  return interpolate(tag, points, pcoords, std::forward<WCoordType>(wcoords));
}

template <typename Points, typename WCoordType, typename PCoordType>
LCL_EXEC inline lcl::ErrorCode worldToParametric(
  Polygon tag,
  const Points& points,
  const WCoordType& wcoords,
  PCoordType&& pcoords) noexcept
{
  LCL_STATIC_ASSERT_PCOORDS_IS_FLOAT_TYPE(PCoordType);

  switch (tag.numberOfPoints())
  {
    case 3:
      return worldToParametric(Triangle{}, points, wcoords, pcoords);
    case 4:
      return worldToParametric(Quad{}, points, wcoords, pcoords);
    default:
      break;
  }

  using T = ComponentType<WCoordType>;
  auto numPoints = tag.numberOfPoints();

  // Find the position of the center point.
  internal::Vector<T, 3> wcoordCenter{T(0), T(0), T(0)};
  for (IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
  {
    wcoordCenter[0] += points.getValue(pointIndex, 0);
    wcoordCenter[1] += points.getValue(pointIndex, 1);
    wcoordCenter[2] += points.getValue(pointIndex, 2);
  }
  wcoordCenter /= static_cast<T>(numPoints);

  // Find the normal vector to the polygon. If the polygon is planar, convex,
  // and in general position, any three points will give a normal in the same
  // direction. Although not perfectly robust, we can reduce the effect of
  // non-planar, non-convex, or degenerate polygons by picking three points
  // topologically far from each other. Note that we do not care about the
  // length of the normal in this case.
  internal::Vector<T, 3> polygonNormal;
  {
    internal::Vector<T, 3> v1p1, v1p2;
    points.getTuple(0, v1p1);
    points.getTuple(numPoints / 3, v1p2);

    internal::Vector<T, 3> v2p1, v2p2;
    points.getTuple(1, v2p1);
    points.getTuple(2 * numPoints / 3, v2p2);

    polygonNormal = internal::cross(v1p2 - v1p1, v2p2 - v2p1);
  }

  // Find which triangle wcoords is located in. We do this by defining the
  // equations for the planes through the radial edges and perpendicular to the
  // polygon. The point is in the triangle if it is on the correct side of both
  // planes.
  internal::Vector<T, 3> wc{ component(wcoords, 0), component(wcoords, 1), component(wcoords, 2) };
  IdComponent firstPointIndex;
  IdComponent secondPointIndex = 0;
  internal::Vector<T, 3> firstPoint, secondPoint;
  bool foundTriangle = false;
  for (firstPointIndex = 0; firstPointIndex < numPoints - 1; ++firstPointIndex)
  {
    points.getTuple(firstPointIndex, firstPoint);
    auto vecInPlane = firstPoint - wcoordCenter;

    auto planeNormal = internal::cross(polygonNormal, vecInPlane);
    auto planeOffset = internal::dot(planeNormal, wcoordCenter);
    if (internal::dot(planeNormal, wc) < planeOffset)
    {
      // wcoords on wrong side of plane, thus outside of triangle
      continue;
    }

    secondPointIndex = firstPointIndex + 1;
    points.getTuple(secondPointIndex, secondPoint);
    vecInPlane = secondPoint - wcoordCenter;

    planeNormal = internal::cross(polygonNormal, vecInPlane);
    planeOffset = internal::dot(planeNormal, wcoordCenter);
    if (internal::dot(planeNormal, wc) > planeOffset)
    {
      // wcoords on wrong side of plane, thus outside of triangle
      continue;
    }

    foundTriangle = true;
    break;
  }
  if (!foundTriangle)
  {
    // wcoord was outside of all triangles we checked. It must be inside the
    // one triangle we did not check (the one between the first and last
    // polygon points).
    firstPointIndex = numPoints - 1;
    points.getTuple(firstPointIndex, firstPoint);
    secondPointIndex = 0;
    points.getTuple(secondPointIndex, secondPoint);
  }

  // Build a structure containing the points of the triangle wcoords is in and
  // use the triangle version of this function to find the parametric
  // coordinates.
  internal::Vector<T, 3> triangleWCoords[3] = { wcoordCenter, firstPoint, secondPoint };
  internal::Vector<T, 3> trianglePCoords;
  LCL_RETURN_ON_ERROR(worldToParametric(
    Triangle{}, makeFieldAccessorNestedSOA(triangleWCoords, 3), wc, trianglePCoords))

  // trianglePCoords is in the triangle's parameter space rather than the
  // polygon's parameter space. We can find the polygon's parameter space by
  // repurposing parametricToWorld by using the
  // polygon parametric coordinates as a proxy for world coordinates.
  LCL_RETURN_ON_ERROR(parametricCenter(tag, triangleWCoords[0]))
  LCL_RETURN_ON_ERROR(parametricPoint(tag, firstPointIndex, triangleWCoords[1]))
  LCL_RETURN_ON_ERROR(parametricPoint(tag, secondPointIndex, triangleWCoords[2]))
  triangleWCoords[0][2] = triangleWCoords[1][2] = triangleWCoords[2][2] = T(0);
  LCL_RETURN_ON_ERROR(
    parametricToWorld(Triangle{}, makeFieldAccessorNestedSOA(triangleWCoords, 3), trianglePCoords, wc))

  component(pcoords, 0) = static_cast<ComponentType<PCoordType>>(wc[0]);
  component(pcoords, 1) = static_cast<ComponentType<PCoordType>>(wc[1]);

  return ErrorCode::SUCCESS;
}

} //namespace lcl

#endif //lcl_Polygon_h
