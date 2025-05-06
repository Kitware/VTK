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

#ifndef viskores_Geometry_h
#define viskores_Geometry_h

#include <viskores/VectorAnalysis.h>

namespace viskores
{

// Forward declarations of geometric types:
template <typename CoordType, int Dim, bool IsTwoSided>
struct Ray;
template <typename CoordType, int Dim>
struct LineSegment;
template <typename CoordType>
struct Plane;
template <typename CoordType, int Dim>
struct Sphere;

/// Represent an infinite or semi-infinite line segment with a point and a direction
///
/// The \a IsTwoSided template parameter indicates whether the class represents
/// an infinite line extending in both directions from the base point or a
/// semi-infinite ray extending only in the positive direction from the base points.
template <typename CoordType = viskores::FloatDefault, int Dim = 3, bool IsTwoSided = false>
struct Ray
{
  static constexpr int Dimension = Dim;
  using Vector = viskores::Vec<CoordType, Dim>;
  static constexpr bool TwoSided = IsTwoSided;

  Vector Origin;
  Vector Direction; // always stored as a unit length.

  /// Construct a default 2-D ray, from (0,0) pointing along the +x axis.
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 2, int>::type = 0>
  VISKORES_EXEC_CONT Ray();

  /// Construct a default 3-D ray from (0,0,0) pointing along the +x axis.
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 3, int>::type = 0>
  VISKORES_EXEC_CONT Ray();

  /// Construct a ray from a point and direction.
  VISKORES_EXEC_CONT
  Ray(const Vector& point, const Vector& direction);

  /// Construct a ray from a line segment.
  VISKORES_EXEC_CONT
  Ray(const LineSegment<CoordType, Dim>& segment);

  /// Return whether the ray is valid or not.
  ///
  /// It is possible for an invalid direction (zero length)
  /// to be passed to the constructor. When this happens,
  /// the constructor divides by zero, leaving Inf in all
  /// components.
  VISKORES_EXEC_CONT
  bool IsValid() const;

  /// Compute a point along the line. \a param values > 0 lie on the ray.
  VISKORES_EXEC_CONT
  Vector Evaluate(CoordType param) const;

  /// Return the minmum distance from \a point to this line/ray.
  ///
  /// Note that when the direction has zero length, this simplifies
  /// the distance between \a point and the ray's origin.
  /// Otherwise, the distance returned is either the perpendicular
  /// distance from \a point to the line or the distance
  /// to the ray's origin.
  VISKORES_EXEC_CONT
  CoordType DistanceTo(const Vector& point) const;

  /// Return the minimum distance between the ray/line and \a point.
  VISKORES_EXEC_CONT
  CoordType DistanceTo(const Vector& point, CoordType& param, Vector& projectedPoint) const;

  /// Compute the non-degenerate point where two 2-D rays intersect, or return false.
  ///
  /// If true is returned, then the rays intersect in a unique point and
  /// \a point is set to that location.
  ///
  /// If false is returned, then either
  /// (1) the rays are parallel and may be
  /// coincident (intersect everywhere) or offset (intersect nowhere); or
  /// (2) the lines intersect but not the rays (because the the intersection
  /// occurs in the negative parameter space of one or both rays).
  /// In the latter case (2), the \a point is still set to the location of the
  /// intersection.
  ///
  /// The tolerance \a tol is the minimum acceptable denominator used to
  /// compute the intersection point coordinates and thus dictates the
  /// maximum distance from the segments at which intersections will be
  /// reported as valid.
  template <bool OtherTwoSided, int Dim_ = Dim, typename std::enable_if<Dim_ == 2, int>::type = 0>
  VISKORES_EXEC_CONT bool Intersect(const Ray<CoordType, Dim, OtherTwoSided>& other,
                                    Vector& point,
                                    CoordType tol = 0.f);
};

/// Represent a finite line segment with a pair of points
template <typename CoordType = viskores::FloatDefault, int Dim = 3>
struct LineSegment
{
  static constexpr int Dimension = Dim;
  using Vector = viskores::Vec<CoordType, Dim>;
  Vector Endpoints[2];

  /// Construct a default segment from (0,0) to (1,0).
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 2, int>::type = 0>
  VISKORES_EXEC_CONT LineSegment();

  /// Construct a default segment from (0,0,0) to (1,0,0).
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 3, int>::type = 0>
  VISKORES_EXEC_CONT LineSegment();

  /// Construct a segment spanning points \a p0 and \a p1.
  VISKORES_EXEC_CONT
  LineSegment(const Vector& p0, const Vector& p1);

  /// Return whether this line segment has an infinitesimal extent (i.e., whether the endpoints are coincident)
  VISKORES_EXEC_CONT
  bool IsSingular(CoordType tol2 = static_cast<CoordType>(1.0e-6f)) const;

  /// Construct a plane bisecting this line segment (only when Dimension is 3).
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 3, int>::type = 0>
  VISKORES_EXEC_CONT Plane<CoordType> PerpendicularBisector() const;

  /// Construct a perpendicular bisector to this line segment (only when Dimension is 2).
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 2, int>::type = 0>
  VISKORES_EXEC_CONT Ray<CoordType, Dim, true> PerpendicularBisector() const;

  /// Return the midpoint of the line segment
  VISKORES_EXEC_CONT
  Vector Center() const { return this->Evaluate(0.5f); }

  /// Return the vector pointing to endpoint 1 from endpoint 0.
  /// This vector is not of unit length and in the case of
  /// degenerate lines, may have zero length.
  /// Call viskores::Normal() on the return value if you want a normalized result.
  VISKORES_EXEC_CONT
  Vector Direction() const { return this->Endpoints[1] - this->Endpoints[0]; }

  /// Compute a point along the line. \a param values in [0,1] lie on the line segment.
  VISKORES_EXEC_CONT
  Vector Evaluate(CoordType param) const;

  /// Return the minmum distance from \a point to this line segment.
  ///
  /// Note that when the endpoints are coincident, this simplifies
  /// the distance between \a point and either endpoint.
  /// Otherwise, the distance returned is either the perpendicular
  /// distance from \a point to the line segment or the distance
  /// to the nearest endpoint (whichever is smaller).
  VISKORES_EXEC_CONT
  CoordType DistanceTo(const Vector& point) const;

  /// Return the minimum distance between the line segment and \a point.
  VISKORES_EXEC_CONT
  CoordType DistanceTo(const Vector& point, CoordType& param, Vector& projectedPoint) const;

  /// Compute the non-degenerate point where two (infinite) 2-D line segments intersect, or return false.
  ///
  /// If true is returned, then the lines intersect in a unique point and
  /// \a point is set to that location.
  ///
  /// If false is returned, then the lines are parallel and either they are
  /// coincident (intersect everywhere) or offset (intersect nowhere).
  ///
  /// The tolerance \a tol is the minimum acceptable denominator used to
  /// compute the intersection point coordinates and thus dictates the
  /// maximum distance from the segments at which intersections will be
  /// reported as valid.
  template <int Dim_ = Dim, typename std::enable_if<Dim_ == 2, int>::type = 0>
  VISKORES_EXEC_CONT bool IntersectInfinite(const LineSegment<CoordType, Dim>& other,
                                            Vector& point,
                                            CoordType tol = 0.f);
};

/// Represent a plane with a base point (origin) and normal vector.
template <typename CoordType = viskores::FloatDefault>
struct Plane
{
  using Vector = viskores::Vec<CoordType, 3>;
  Vector Origin;
  Vector Normal;

  /// Construct a default plane whose base point is the origin and whose normal is (0,0,1)
  VISKORES_EXEC_CONT
  Plane();

  /// Construct a plane with the given \a origin and \a normal.
  VISKORES_EXEC_CONT
  Plane(const Vector& origin, const Vector& normal, CoordType tol2 = static_cast<CoordType>(1e-8f));

  /// Return true if the plane's normal is well-defined to within the given tolerance.
  VISKORES_EXEC_CONT
  bool IsValid() const { return !viskores::IsInf(this->Normal[0]); }

  /// Return the **signed** distance from the plane to the point.
  VISKORES_EXEC_CONT
  CoordType DistanceTo(const Vector& point) const;

  /// Return the closest point in the plane to the given point.
  VISKORES_EXEC_CONT
  Vector ClosestPoint(const Vector& point) const;

  /// Intersect this plane with the ray (or line if the ray is two-sided).
  ///
  /// Returns true if there is a non-degenrate intersection (i.e., an isolated point of intersection).
  /// Returns false if there is no intersection *or* if the intersection is degenerate (i.e., the
  /// entire ray/line lies in the plane).
  /// In the latter case, \a lineInPlane will be true upon exit.
  ///
  /// If this method returns true, then \a parameter will be set to a number indicating
  /// where along the ray/line  the plane hits and \a point will be set to that location.
  /// If the input is a ray, the \a parameter will be non-negative.
  template <bool IsTwoSided>
  VISKORES_EXEC_CONT bool Intersect(const Ray<CoordType, 3, IsTwoSided>& ray,
                                    CoordType& parameter,
                                    Vector& point,
                                    bool& lineInPlane,
                                    CoordType tol = CoordType(1e-6f)) const;

  /// Intersect this plane with the line \a segment.
  ///
  /// Returns true if there is a non-degenrate intersection (i.e., an isolated point of intersection).
  /// Returns false if there is no intersection *or* if the intersection is degenerate (i.e., the
  /// entire line segment lies in the plane).
  /// In the latter case, \a lineInPlane will be true upon exit.
  ///
  /// If this method returns true, then \a parameter will be set to a number in [0,1] indicating
  /// where along the line segment the plane hits.
  VISKORES_EXEC_CONT
  bool Intersect(const LineSegment<CoordType>& segment,
                 CoordType& parameter,
                 bool& lineInPlane) const;

  /// Intersect this plane with the line \a segment.
  ///
  /// Returns true if there is a non-degenrate intersection (i.e., an isolated point of intersection).
  /// Returns false if there is no intersection *or* if the intersection is degenerate (i.e., the
  /// entire line segment lines in the plane).
  /// In the latter case, \a lineInPlane will be true upon exit.
  ///
  /// If this method returns true, then \a parameter will be set to a number in [0,1] indicating
  /// where along the line segment the plane hits and \a point will be set to that location.
  VISKORES_EXEC_CONT
  bool Intersect(const LineSegment<CoordType>& segment,
                 CoordType& parameter,
                 Vector& point,
                 bool& lineInPlane) const;

  /// Intersect this plane with another plane.
  ///
  /// Returns true if there is a non-degenrate intersection (i.e., a line of intersection).
  /// Returns false if there is no intersection *or* if the intersection is degenerate
  /// (i.e., the planes are coincident).
  /// In the latter case, \a coincident will be true upon exit and \a segment will
  /// unmodified.
  ///
  /// If this method returns true, then the resulting \a segment will have its
  /// base point on the line of intersection and its second point will be a unit
  /// length away in the direction of the cross produce of the input plane normals
  /// (this plane crossed with the \a other).
  ///
  /// The tolerance \a tol is the minimum squared length of the cross-product
  /// of the two plane normals. It is also compared to the squared distance of
  /// the base point of \a other away from \a this plane when considering whether
  /// the planes are coincident.
  VISKORES_EXEC_CONT
  bool Intersect(const Plane<CoordType>& other,
                 Ray<CoordType, 3, true>& ray,
                 bool& coincident,
                 CoordType tol2 = static_cast<CoordType>(1e-6f)) const;
};

/// Represent a sphere of the given \a Dimension.
/// If a constructor is given an invalid specification, then
/// the Radius of the resulting sphere will be -1.
template <typename CoordType = viskores::FloatDefault, int Dim = 3>
struct Sphere
{
  static constexpr int Dimension = Dim;
  using Vector = viskores::Vec<CoordType, Dim>;
  Vector Center;
  CoordType Radius;

  /// Construct a default sphere (unit radius at the origin).
  VISKORES_EXEC_CONT Sphere();

  /// Construct a sphere from a center point and radius.
  VISKORES_EXEC_CONT Sphere(const Vector& center, CoordType radius);

  /// Return true if the sphere is valid (i.e., has a strictly positive radius).
  VISKORES_EXEC_CONT
  bool IsValid() const { return this->Radius > 0.f; }

  /// Return whether the point lies strictly inside the sphere.
  VISKORES_EXEC_CONT
  bool Contains(const Vector& point, CoordType tol2 = 0.f) const;

  /// Classify a point as inside (-1), on (0), or outside (+1) of the sphere.
  ///
  /// The tolerance \a tol2 is the maximum allowable difference in squared
  /// magnitude between the squared radius and the squared distance between
  /// the \a point and Center.
  VISKORES_EXEC_CONT
  int Classify(const Vector& point, CoordType tol2 = 0.f) const;
};

// -----------------------------------------------------------------------------
// Synonyms
//
// These "using" statements aim to make it easier to use the templated
// structs above when working with a particular dimension and/or the
// default floating-point type.

/// Lines are two-sided rays:
template <typename CoordType, int Dim = 3>
using Line = Ray<CoordType, Dim, true>;

// Shortcuts for 2D and 3D rays, lines, and line segments:
template <typename CoordType>
using Ray2 = Ray<CoordType, 2>;
template <typename CoordType>
using Ray3 = Ray<CoordType, 3>;
template <typename CoordType>
using Line2 = Line<CoordType, 2>;
template <typename CoordType>
using Line3 = Line<CoordType, 3>;
template <typename CoordType>
using LineSegment2 = LineSegment<CoordType, 2>;
template <typename CoordType>
using LineSegment3 = LineSegment<CoordType, 3>;

/// Circle is an alias for a 2-Dimensional sphere.
template <typename T>
using Circle = Sphere<T, 2>;

// Aliases for d-dimensional spheres.
template <typename T>
using Sphere2 = Sphere<T, 2>;
template <typename T>
using Sphere3 = Sphere<T, 3>;

// Shortcuts for default floating-point types
using Ray2d = Ray2<viskores::FloatDefault>;
using Ray3d = Ray3<viskores::FloatDefault>;
using Line2d = Line2<viskores::FloatDefault>;
using Line3d = Line3<viskores::FloatDefault>;
using LineSegment2d = LineSegment2<viskores::FloatDefault>;
using LineSegment3d = LineSegment3<viskores::FloatDefault>;
using Plane3d = Plane<viskores::FloatDefault>;
using Circle2d = Circle<viskores::FloatDefault>;
using Sphere2d = Sphere2<viskores::FloatDefault>;
using Sphere3d = Sphere3<viskores::FloatDefault>;

// -----------------------------------------------------------------------------
// Construction techniques
//
// These are free functions that create instances of geometric structs by taking
// in data that is not identical to the state of the struct and converting it
// into state for the struct.

/// Construct a plane from a point plus one of: a line, a ray, or a line segment.
///
/// The plane returned will contain the point and the line/ray/segment.
/// The plane normal will be the cross product of the line/ray/segment's direction
/// and the vector from the line/ray/segment's origin to the given \a point.
/// If the \a point is collinear with the line/ray/line-segment, an invalid
/// plane will be returned.
template <typename CoordType, bool IsTwoSided>
VISKORES_EXEC_CONT viskores::Plane<CoordType> make_PlaneFromPointAndLine(
  const viskores::Vec<CoordType, 3>& point,
  const viskores::Ray<CoordType, 3, IsTwoSided>& ray,
  CoordType tol2 = static_cast<CoordType>(1e-8f));

template <typename CoordType>
VISKORES_EXEC_CONT viskores::Plane<CoordType> make_PlaneFromPointAndLineSegment(
  const viskores::Vec<CoordType, 3>& point,
  const viskores::LineSegment3<CoordType>& segment,
  CoordType tol2 = static_cast<CoordType>(1e-8f));

/// Construct a circle from 3 points.
template <typename CoordType>
VISKORES_EXEC_CONT viskores::Circle<CoordType> make_CircleFrom3Points(
  const typename viskores::Vec<CoordType, 2>& p0,
  const typename viskores::Vec<CoordType, 2>& p1,
  const typename viskores::Vec<CoordType, 2>& p2,
  CoordType tol = static_cast<CoordType>(1e-6f));

/// Construct a sphere from 4 points.
template <typename CoordType>
VISKORES_EXEC_CONT viskores::Sphere<CoordType, 3> make_SphereFrom4Points(
  const viskores::Vec<CoordType, 3>& a0,
  const viskores::Vec<CoordType, 3>& a1,
  const viskores::Vec<CoordType, 3>& a2,
  const viskores::Vec<CoordType, 3>& a3,
  CoordType tol = static_cast<CoordType>(1e-6f));

} // namespace viskores

#include <viskores/Geometry.hxx>

#endif // viskores_Geometry_h
