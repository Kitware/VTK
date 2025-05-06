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

#include <viskores/Geometry.h>
#include <viskores/Math.h>

#include <viskores/TypeList.h>
#include <viskores/VecTraits.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/cont/Algorithm.h>

#include <viskores/cont/testing/Testing.h>

//-----------------------------------------------------------------------------
namespace
{

class Coords
{
public:
  static constexpr viskores::IdComponent NUM_COORDS = 5;

  template <typename T>
  VISKORES_EXEC_CONT viskores::Vec<T, 3> EndpointList(viskores::Int32 i) const
  {
    viskores::Float64 coords[NUM_COORDS][3] = {
      { 1.0, 0.0, 0.0 },  { 0.0, 1.0, 0.0 },  { -1.0, 0.0, 0.0 },
      { -2.0, 0.0, 0.0 }, { 0.0, -2.0, 0.0 },
    };
    viskores::Int32 j = i % NUM_COORDS;
    return viskores::Vec<T, 3>(
      static_cast<T>(coords[j][0]), static_cast<T>(coords[j][1]), static_cast<T>(coords[j][2]));
  }

  template <typename T>
  VISKORES_EXEC_CONT viskores::Vec<T, 3> ClosestToOriginList(viskores::Int32 i) const
  {
    viskores::Float64 coords[NUM_COORDS][3] = {
      { 0.5, 0.5, 0.0 },   { -0.5, 0.5, 0.0 }, { -1.0, 0.0, 0.0 },
      { -1.0, -1.0, 0.0 }, { 0.8, -0.4, 0.0 },
    };
    viskores::Int32 j = i % NUM_COORDS;
    return viskores::Vec<T, 3>(
      static_cast<T>(coords[j][0]), static_cast<T>(coords[j][1]), static_cast<T>(coords[j][2]));
  }

  template <typename T>
  VISKORES_EXEC_CONT T DistanceToOriginList(viskores::Int32 i) const
  {
    viskores::Float64 coords[NUM_COORDS] = {
      0.707107, 0.707107, 1.0, 1.41421, 0.894427,
    };
    viskores::Int32 j = i % NUM_COORDS;
    return static_cast<T>(coords[j]);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct RayTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    {
      using V2 = viskores::Vec<T, 2>;
      using Ray2 = viskores::Ray2<T>;

      Ray2 ray0;
      VISKORES_MATH_ASSERT(test_equal(ray0.Origin, V2(0., 0.)),
                           "Bad origin for default 2D ray ctor.");
      VISKORES_MATH_ASSERT(test_equal(ray0.Direction, V2(1., 0.)),
                           "Bad direction for default 2D ray ctor.");

      // Test intersection
      Ray2 ray1(V2(-1., 0.), V2(+1., +1.));
      Ray2 ray2(V2(+1., 0.), V2(-1., +1.));
      V2 point;
      bool didIntersect = ray1.Intersect(ray2, point);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true), "Ray-pair 1 should intersect.");
      VISKORES_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 1 should intersect at (0,1).");

      // Test non intersection
      Ray2 ray3(V2(-1., 0.), V2(-1., -1.));
      Ray2 ray4(V2(+1., 0.), V2(+1., -1.));

      didIntersect = ray1.Intersect(ray4, point);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 2 should not intersect.");
      VISKORES_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 2 should intersect at (0,1).");

      didIntersect = ray3.Intersect(ray2, point);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 3 should not intersect.");
      VISKORES_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 3 should intersect at (0,1).");

      didIntersect = ray3.Intersect(ray4, point);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false), "Ray-pair 4 should not intersect.");
      VISKORES_MATH_ASSERT(test_equal(point, V2(0., 1.)), "Ray-pair 4 should intersect at (0,1).");
    }

    {
      using V3 = viskores::Vec<T, 3>;

      viskores::Ray<T, 3> ray0;
      VISKORES_MATH_ASSERT(test_equal(ray0.Origin, V3(0., 0., 0.)),
                           "Bad origin for default 3D ray ctor.");
      VISKORES_MATH_ASSERT(test_equal(ray0.Direction, V3(1., 0., 0.)),
                           "Bad direction for default 3D ray ctor.");
    }
  }
};

struct TryRayTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(RayTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct LineSegmentTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    {
      using V2 = viskores::Vec<T, 2>;
      using Line2 = viskores::Line2<T>;

      viskores::LineSegment<T, 2> seg0;
      VISKORES_MATH_ASSERT(test_equal(seg0.Endpoints[0], V2(0., 0.)),
                           "Bad origin for default 2D line segment ctor.");
      VISKORES_MATH_ASSERT(test_equal(seg0.Endpoints[1], V2(1., 0.)),
                           "Bad direction for default 2D line segment ctor.");

      V2 p0(1., 1.);
      V2 p1(3., 3.);
      V2 p2(2., 2.);
      // V2 p3(static_cast<T>(1.2928932), static_cast<T>(2.7071068));
      V2 dir(static_cast<T>(-0.7071068), static_cast<T>(0.7071068));
      viskores::LineSegment<T, 2> seg1(p0, p1);
      Line2 ray = seg1.PerpendicularBisector();
      VISKORES_MATH_ASSERT(test_equal(ray.Origin, p2),
                           "Perpendicular bisector origin failed in 2D.");
      VISKORES_MATH_ASSERT(test_equal(ray.Direction, dir),
                           "Perpendicular bisector direction failed in 2D.");
    }

    {
      using V3 = viskores::Vec<T, 3>;

      viskores::LineSegment<T, 3> seg0;
      VISKORES_MATH_ASSERT(test_equal(seg0.Endpoints[0], V3(0., 0., 0.)),
                           "Bad origin for default 3D line segment ctor.");
      VISKORES_MATH_ASSERT(test_equal(seg0.Endpoints[1], V3(1., 0., 0.)),
                           "Bad direction for default 3D line segment ctor.");

      V3 p0(1., 1., 0.);
      V3 p1(3., 3., 0.);
      V3 p2(2., 2., 0.);
      V3 p3(static_cast<T>(0.70710678), static_cast<T>(0.70710678), 0.);
      viskores::LineSegment<T, 3> seg1(p0, p1);
      viskores::Plane<T> bisector = seg1.PerpendicularBisector();
      VISKORES_MATH_ASSERT(test_equal(bisector.Origin, p2),
                           "Perpendicular bisector origin failed in 3D.");
      VISKORES_MATH_ASSERT(test_equal(bisector.Normal, p3),
                           "Perpendicular bisector direction failed in 3D.");
    }

    viskores::Vec<T, 3> origin(0., 0., 0.);
    for (viskores::IdComponent index = 0; index < Coords::NUM_COORDS; ++index)
    {
      auto p0 = Coords{}.EndpointList<T>(index);
      auto p1 = Coords{}.EndpointList<T>((index + 1) % Coords::NUM_COORDS);

      viskores::LineSegment<T> segment(p0, p1);
      viskores::Vec<T, 3> closest;
      T param;
      auto dp0 = segment.DistanceTo(p0);
      auto dp1 = segment.DistanceTo(p1, param, closest);
      VISKORES_MATH_ASSERT(test_equal(dp0, 0.0), "Distance to endpoint 0 not zero.");
      VISKORES_MATH_ASSERT(test_equal(dp1, 0.0), "Distance to endpoint 1 not zero.");
      VISKORES_MATH_ASSERT(test_equal(param, 1.0), "Parameter value of endpoint 1 not 1.0.");
      VISKORES_MATH_ASSERT(test_equal(p1, closest), "Closest point not endpoint 1.");

      closest = segment.Evaluate(static_cast<T>(0.0));
      VISKORES_MATH_ASSERT(test_equal(p0, closest), "Evaluated point not endpoint 0.");

      auto dpo = segment.DistanceTo(origin, param, closest);
      auto clo = Coords{}.ClosestToOriginList<T>(index);
      auto dst = Coords{}.DistanceToOriginList<T>(index);
      VISKORES_MATH_ASSERT(test_equal(closest, clo), "Closest point to origin doesn't match.");
      VISKORES_MATH_ASSERT(test_equal(dpo, dst), "Distance to origin doesn't match.");
    }
  }
};

struct TryLineSegmentTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(LineSegmentTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct PlaneTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    viskores::Vec<T, 3> origin(0., 0., 0.);
    viskores::Vec<T, 3> zvectr(0., 0., 5.); // intentionally not unit length to test normalization.
    viskores::Plane<T> plane;
    viskores::LineSegment<T> segment;
    T dist;
    bool didIntersect;
    bool isLineInPlane;
    viskores::Vec<T, 3> nearest;
    viskores::Vec<T, 3> p0;
    viskores::Vec<T, 3> p1;
    T param;

    // Test signed plane-point distance
    plane = viskores::Plane<T>(origin, zvectr);
    dist = plane.DistanceTo(viskores::Vec<T, 3>(82., 0.5, 1.25));
    VISKORES_MATH_ASSERT(test_equal(dist, 1.25), "Bad positive point-plane distance.");
    dist = plane.DistanceTo(viskores::Vec<T, 3>(82., 0.5, -1.25));
    VISKORES_MATH_ASSERT(test_equal(dist, -1.25), "Bad negative point-plane distance.");
    dist = plane.DistanceTo(viskores::Vec<T, 3>(82., 0.5, 0.0));
    VISKORES_MATH_ASSERT(test_equal(dist, 0.0), "Bad zero point-plane distance.");

    // Test line intersection
    {
      // Case 1. No intersection
      segment = viskores::LineSegment<T>((p0 = viskores::Vec<T, 3>(1., 1., 1.)),
                                         (p1 = viskores::Vec<T, 3>(2., 2., 2.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false),
                           "Plane and line should not intersect (1).");
      VISKORES_MATH_ASSERT(test_equal(isLineInPlane, false),
                           "Line improperly reported as in plane (1).");
      VISKORES_MATH_ASSERT(test_equal(nearest, p0), "Unexpected nearest point (1).");
      VISKORES_MATH_ASSERT(test_equal(param, 0.0), "Unexpected nearest parameter value (1).");

      // Case 2. Degenerate intersection (entire segment lies in plane)
      segment = viskores::LineSegment<T>((p0 = viskores::Vec<T, 3>(1., 1., 0.)),
                                         (p1 = viskores::Vec<T, 3>(2., 2., 0.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (2).");
      VISKORES_MATH_ASSERT(test_equal(isLineInPlane, true),
                           "Line improperly reported as out of plane (2).");

      // Case 3. Endpoint intersection
      segment = viskores::LineSegment<T>((p0 = viskores::Vec<T, 3>(1., 1., 1.)),
                                         (p1 = viskores::Vec<T, 3>(2., 2., 0.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (3a).");
      VISKORES_MATH_ASSERT(test_equal(isLineInPlane, false),
                           "Line improperly reported as in plane (3a).");
      VISKORES_MATH_ASSERT(test_equal(param, 1.0),
                           "Invalid parameter for intersection point (3a).");
      VISKORES_MATH_ASSERT(test_equal(nearest, p1), "Invalid intersection point (3a).");

      segment = viskores::LineSegment<T>((p0 = viskores::Vec<T, 3>(1., 1., 0.)),
                                         (p1 = viskores::Vec<T, 3>(2., 2., 1.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (3b).");
      VISKORES_MATH_ASSERT(test_equal(isLineInPlane, false),
                           "Line improperly reported as in plane (3b).");
      VISKORES_MATH_ASSERT(test_equal(param, 0.0),
                           "Invalid parameter for intersection point (3b).");
      VISKORES_MATH_ASSERT(test_equal(nearest, p0), "Invalid intersection point (3b).");

      // Case 4. General-position intersection
      segment = viskores::LineSegment<T>((p0 = viskores::Vec<T, 3>(-1., -1., -1.)),
                                         (p1 = viskores::Vec<T, 3>(2., 2., 1.)));
      didIntersect = plane.Intersect(segment, param, nearest, isLineInPlane);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true), "Plane and line should intersect (4).");
      VISKORES_MATH_ASSERT(test_equal(isLineInPlane, false),
                           "Line improperly reported as in plane (4).");
      VISKORES_MATH_ASSERT(test_equal(param, 0.5), "Invalid parameter for intersection point (4).");
      VISKORES_MATH_ASSERT(test_equal(nearest, viskores::Vec<T, 3>(0.5, 0.5, 0)),
                           "Invalid intersection point (4).");
    }

    // Test plane-plane intersection
    {
      using V3 = viskores::Vec<T, 3>;
      using PlaneType = viskores::Plane<T>;
      // Case 1. Coincident planes
      p0 = V3(1., 2., 3.);
      p1 = V3(5., 7., -6.);
      V3 nn = viskores::Normal(V3(1., 1., 1));
      PlaneType pa(p0, nn);
      PlaneType pb(p1, nn);
      viskores::Line3<T> ii;
      bool coincident;
      didIntersect = pa.Intersect(pb, ii, coincident);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false),
                           "Coincident planes should have degenerate intersection.");
      VISKORES_MATH_ASSERT(test_equal(coincident, true),
                           "Coincident planes should be marked coincident.");

      // Case 2. Offset planes
      p1 = V3(5., 6., 7.);
      pb = PlaneType(p1, nn);
      didIntersect = pa.Intersect(pb, ii, coincident);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, false),
                           "Offset planes should have degenerate intersection.");
      VISKORES_MATH_ASSERT(test_equal(coincident, false),
                           "Offset planes should not be marked coincident.");

      // Case 3. General position
      p1 = V3(1., 2., 0.);
      V3 n2(0., 0., 1.);
      pb = PlaneType(p1, n2);
      didIntersect = pa.Intersect(pb, ii, coincident);
      VISKORES_MATH_ASSERT(test_equal(didIntersect, true),
                           "Proper planes should have non-degenerate intersection.");
      VISKORES_MATH_ASSERT(test_equal(coincident, false),
                           "Proper planes should not be marked coincident.");
      VISKORES_MATH_ASSERT(test_equal(ii.Origin, V3(2.5, 3.5, 0)),
                           "Unexpected intersection-line base point.");
      VISKORES_MATH_ASSERT(test_equal(ii.Direction, viskores::Normal(V3(1, -1, 0))),
                           "Unexpected intersection-line direction.");
    }
  }
};

struct TryPlaneTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(PlaneTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------

template <typename T>
struct SphereTests : public viskores::exec::FunctorBase
{
  VISKORES_EXEC
  void operator()(viskores::Id) const
  {
    {
      using V2 = viskores::Vec<T, 2>;
      V2 origin(0., 0.);
      viskores::Sphere<T, 2> defaultSphere;
      VISKORES_MATH_ASSERT(test_equal(defaultSphere.Center, origin),
                           "Default circle not at origin.");
      VISKORES_MATH_ASSERT(test_equal(defaultSphere.Radius, 1.0),
                           "Default circle not unit radius.");

      viskores::Sphere<T, 2> sphere(origin, -2.);
      VISKORES_MATH_ASSERT(test_equal(sphere.Radius, -1.0),
                           "Negative radius should be reset to -1.");
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), false),
                           "Negative radius should leave sphere invalid.");

      sphere = viskores::Circle<T>(origin, 1.0);
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Circle assignment failed.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Contains(origin), true),
                           "Circle does not contain its center.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Classify(V2(1., 0.)), 0),
                           "Circle point not on boundary.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Classify(V2(0.75, 0.75)), +1),
                           "Circle contains a point that should be outside.");

      V2 p0(static_cast<T>(-0.7071), static_cast<T>(-0.7071));
      V2 p1(static_cast<T>(+0.7071), static_cast<T>(-0.7071));
      V2 p2(static_cast<T>(0.0), static_cast<T>(1.0));
      sphere = make_CircleFrom3Points(p0, p1, p2);
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Could not create 3-point circle.");

      V2 p3(1, 1);
      V2 p4(3, 4);
      V2 p5(5, 12);
      sphere = make_CircleFrom3Points(p3, p4, p5);
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Could not create 3-point circle.");
      T tol = static_cast<T>(1e-3); // Use a loose tolerance
      VISKORES_MATH_ASSERT(test_equal(sphere.Center, viskores::Vec<T, 2>(-12.4f, 12.1f)),
                           "Invalid circle center.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Radius, static_cast<T>(17.400291f)),
                           "Invalid circle radius.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Classify(p3, tol), 0),
                           "Generator p3 not on circle boundary.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Classify(p4, tol), 0),
                           "Generator p4 not on circle boundary.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Classify(p5, tol), 0),
                           "Generator p5 not on circle boundary.");

      V2 p6(1, 1);
      V2 p7(4, 4);
      V2 p8(5, 5);
      sphere = make_CircleFrom3Points(p6, p7, p8);
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), false),
                           "3-point circle construction should fail with points on a line.");
    }
    {
      using V3 = viskores::Vec<T, 3>;

      V3 p0(0., 1., 0.);
      V3 p1(1., 0., 0.);
      V3 p2(-1., 0., 0.);
      V3 p3(0., 0., 1.);
      V3 p4 = viskores::Normal(V3(1., 1., 1.));

      V3 origin(0., 0., 0.);
      viskores::Sphere<T, 3> defaultSphere;
      VISKORES_MATH_ASSERT(test_equal(defaultSphere.Center, origin),
                           "Default sphere not at origin.");
      VISKORES_MATH_ASSERT(test_equal(defaultSphere.Radius, 1.0),
                           "Default sphere not unit radius.");

      viskores::Sphere<T, 3> sphere =
        make_SphereFrom4Points(p0, p1, p2, p3, static_cast<T>(1.0e-6));
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Easy sphere 1 not valid.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Center, origin), "Easy sphere 1 not at origin.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Radius, 1.0), "Easy sphere 1 not unit radius.");

      sphere = make_SphereFrom4Points(p0, p1, p2, p4, static_cast<T>(1.0e-6));
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Easy sphere 2 not valid.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Center, origin), "Easy sphere 2 not at origin.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Radius, 1.0), "Easy sphere 2 not unit radius.");

      V3 fancyCenter(1, 2, 3);
      T fancyRadius(2.5);

      V3 fp0 = fancyCenter + fancyRadius * p0;
      V3 fp1 = fancyCenter + fancyRadius * p1;
      V3 fp2 = fancyCenter + fancyRadius * p2;
      V3 fp4 = fancyCenter + fancyRadius * p4;

      sphere = make_SphereFrom4Points(fp0, fp1, fp2, fp4, static_cast<T>(1.0e-6));
      VISKORES_MATH_ASSERT(test_equal(sphere.IsValid(), true), "Medium sphere 1 not valid.");
      VISKORES_MATH_ASSERT(test_equal(sphere.Center, fancyCenter),
                           "Medium sphere 1 not at (1,2,3).");
      VISKORES_MATH_ASSERT(test_equal(sphere.Radius, fancyRadius),
                           "Medium sphere 1 not radius 2.5.");
    }
  }
};

struct TrySphereTests
{
  template <typename T>
  void operator()(const T&) const
  {
    viskores::cont::Algorithm::Schedule(SphereTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
void RunGeometryTests()
{
  std::cout << "Tests for rays." << std::endl;
  viskores::testing::Testing::TryTypes(TryRayTests(), viskores::TypeListFieldScalar());
  std::cout << "Tests for line segments." << std::endl;
  viskores::testing::Testing::TryTypes(TryLineSegmentTests(), viskores::TypeListFieldScalar());
  std::cout << "Tests for planes." << std::endl;
  viskores::testing::Testing::TryTypes(TryPlaneTests(), viskores::TypeListFieldScalar());
  std::cout << "Tests for spheres." << std::endl;
  viskores::testing::Testing::TryTypes(TrySphereTests(), viskores::TypeListFieldScalar());
}

} // anonymous namespace

int UnitTestGeometry(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunGeometryTests, argc, argv);
}
