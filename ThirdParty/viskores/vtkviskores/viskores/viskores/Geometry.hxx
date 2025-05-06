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

namespace viskores
{

// -----------------------------------------------------------------------------
// Ray

template <typename CoordType, int Dim, bool IsTwoSided>
template <int Dim_, typename std::enable_if<Dim_ == 2, int>::type>
VISKORES_EXEC_CONT Ray<CoordType, Dim, IsTwoSided>::Ray()
  : Origin{ 0.f }
  , Direction{ 1.f, 0.f }
{
}

template <typename CoordType, int Dim, bool IsTwoSided>
template <int Dim_, typename std::enable_if<Dim_ == 3, int>::type>
VISKORES_EXEC_CONT Ray<CoordType, Dim, IsTwoSided>::Ray()
  : Origin{ 0.f }
  , Direction{ 1.f, 0.f, 0.f }
{
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT Ray<CoordType, Dim, IsTwoSided>::Ray(const LineSegment<CoordType, Dim>& segment)
  : Origin(segment.Endpoints[0])
  , Direction(viskores::Normal(segment.Direction()))
{
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT Ray<CoordType, Dim, IsTwoSided>::Ray(const Vector& point,
                                                        const Vector& direction)
  : Origin(point)
  , Direction(viskores::Normal(direction))
{
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT typename Ray<CoordType, Dim, IsTwoSided>::Vector
Ray<CoordType, Dim, IsTwoSided>::Evaluate(CoordType param) const
{
  auto pointOnLine = this->Origin + this->Direction * param;
  return pointOnLine;
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT bool Ray<CoordType, Dim, IsTwoSided>::IsValid() const
{
  return !viskores::IsInf(this->Direction[0]);
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT CoordType Ray<CoordType, Dim, IsTwoSided>::DistanceTo(const Vector& point) const
{
  Vector closest;
  CoordType param;
  return this->DistanceTo(point, param, closest);
}

template <typename CoordType, int Dim, bool IsTwoSided>
VISKORES_EXEC_CONT CoordType
Ray<CoordType, Dim, IsTwoSided>::DistanceTo(const Vector& point,
                                            CoordType& param,
                                            Vector& projectedPoint) const
{
  const auto& dir = this->Direction;
  auto mag2 = viskores::MagnitudeSquared(dir);
  if (mag2 <= static_cast<CoordType>(0.0f))
  {
    // We have a point, not a line segment.
    projectedPoint = this->Origin;
    param = static_cast<CoordType>(0.0f);
    return viskores::Magnitude(point - this->Origin);
  }

  // Find the closest point on the line, then clamp to the ray if the
  // parameter value is negative.
  param = viskores::Dot(point - this->Origin, dir) / mag2;
  if (!TwoSided)
  {
    param = viskores::Max(param, static_cast<CoordType>(0.0f));
  }

  // Compute the distance between the closest point and the input point.
  projectedPoint = this->Evaluate(param);
  auto dist = viskores::Magnitude(point - projectedPoint);
  return dist;
}

template <typename CoordType, int Dim, bool IsTwoSided>
template <bool OtherTwoSided, int Dim_, typename std::enable_if<Dim_ == 2, int>::type>
VISKORES_EXEC_CONT bool Ray<CoordType, Dim, IsTwoSided>::Intersect(
  const Ray<CoordType, Dim, OtherTwoSided>& other,
  Vector& point,
  CoordType tol)
{
  auto d1 = this->Direction;
  auto d2 = other.Direction;
  auto denom = d1[0] * d2[1] - d1[1] * d2[0];
  if (viskores::Abs(denom) < tol)
  { // The lines are coincident or at least parallel.
    return false;
  }
  const auto& a = this->Origin;
  const auto& b = other.Origin;
  CoordType numerU = a[1] * d2[0] + d2[1] * b[0] - b[1] * d2[0] - d2[1] * a[0];

  CoordType uParam = numerU / denom;
  point = a + uParam * d1;
  if (TwoSided && OtherTwoSided)
  {
    return true;
  }
  else
  {
    CoordType numerV = d1[0] * (a[1] - b[1]) - d1[1] * (a[0] - b[0]);
    CoordType vParam = numerV / denom;
    return (TwoSided || (uParam + tol) > 0) && (OtherTwoSided || (vParam + tol) > 0);
  }
}

// -----------------------------------------------------------------------------
// LineSegment

template <typename CoordType, int Dim>
template <int Dim_, typename std::enable_if<Dim_ == 2, int>::type>
VISKORES_EXEC_CONT LineSegment<CoordType, Dim>::LineSegment()
  : Endpoints{ { 0.f }, { 1.f, 0.f } }
{
}

template <typename CoordType, int Dim>
template <int Dim_, typename std::enable_if<Dim_ == 3, int>::type>
VISKORES_EXEC_CONT LineSegment<CoordType, Dim>::LineSegment()
  : Endpoints{ { 0.f }, { 1.f, 0.f, 0.f } }
{
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT LineSegment<CoordType, Dim>::LineSegment(const Vector& p0, const Vector& p1)
  : Endpoints{ p0, p1 }
{
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT bool LineSegment<CoordType, Dim>::IsSingular(CoordType tol2) const
{
  return viskores::MagnitudeSquared(this->Direction()) < tol2;
}

template <typename CoordType, int Dim>
template <int Dim_, typename std::enable_if<Dim_ == 2, int>::type>
VISKORES_EXEC_CONT Ray<CoordType, Dim, true> LineSegment<CoordType, Dim>::PerpendicularBisector()
  const
{
  const Vector dir = this->Direction();
  const Vector perp(-dir[1], dir[0]);
  const Vector mid = this->Center();
  return Ray<CoordType, Dim, true>(mid, perp);
}

template <typename CoordType, int Dim>
template <int Dim_, typename std::enable_if<Dim_ == 3, int>::type>
VISKORES_EXEC_CONT Plane<CoordType> LineSegment<CoordType, Dim>::PerpendicularBisector() const
{
  return Plane<CoordType>(this->Center(), this->Direction());
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT typename LineSegment<CoordType, Dim>::Vector
LineSegment<CoordType, Dim>::Evaluate(CoordType param) const
{
  auto pointOnLine = this->Endpoints[0] * (1.0f - param) + this->Endpoints[1] * param;
  return pointOnLine;
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT CoordType LineSegment<CoordType, Dim>::DistanceTo(const Vector& point) const
{
  Vector closest;
  CoordType param;
  return this->DistanceTo(point, param, closest);
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT CoordType LineSegment<CoordType, Dim>::DistanceTo(const Vector& point,
                                                                     CoordType& param,
                                                                     Vector& projectedPoint) const
{
  auto dir = this->Endpoints[1] - this->Endpoints[0];
  auto mag2 = viskores::MagnitudeSquared(dir);
  if (mag2 <= static_cast<CoordType>(0.0f))
  {
    // We have a point, not a line segment.
    projectedPoint = this->Endpoints[0];
    param = static_cast<CoordType>(0.0f);
    return viskores::Magnitude(point - this->Endpoints[0]);
  }

  // Find the closest point on the line, then clamp to the line segment
  param = viskores::Clamp(viskores::Dot(point - this->Endpoints[0], dir) / mag2,
                          static_cast<CoordType>(0.0f),
                          static_cast<CoordType>(1.0f));

  // Compute the distance between the closest point and the input point.
  projectedPoint = this->Evaluate(param);
  auto dist = viskores::Magnitude(point - projectedPoint);
  return dist;
}

template <typename CoordType, int Dim>
template <int Dim_, typename std::enable_if<Dim_ == 2, int>::type>
VISKORES_EXEC_CONT bool LineSegment<CoordType, Dim>::IntersectInfinite(
  const LineSegment<CoordType, Dim>& other,
  Vector& point,
  CoordType tol)
{
  auto d1 = this->Direction();
  auto d2 = other.Direction();
  auto denom = d1[0] * d2[1] - d1[1] * d2[0];
  if (viskores::Abs(denom) < tol)
  { // The lines are coincident or at least parallel.
    return false;
  }
  const auto& a = this->Endpoints;
  const auto& b = other.Endpoints;
  CoordType numerX = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * -d2[0] -
    (b[0][0] * b[1][1] - b[0][1] * b[1][0]) * -d1[0];
  CoordType numerY = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * -d2[1] -
    (b[0][0] * b[1][1] - b[0][1] * b[1][0]) * -d1[1];
  point = Vector(numerX / denom, numerY / denom);
  return true;
}

// -----------------------------------------------------------------------------
// Plane

template <typename CoordType>
VISKORES_EXEC_CONT VISKORES_EXEC_CONT Plane<CoordType>::Plane()
  : Origin{ 0.f, 0.f, 0.f }
  , Normal{ 0.f, 0.f, 1.f }
{
}

template <typename CoordType>
VISKORES_EXEC_CONT Plane<CoordType>::Plane(const Vector& origin,
                                           const Vector& normal,
                                           CoordType tol2)
  : Origin(origin)
  , Normal(viskores::Normal(normal))
{
  if (tol2 > 0.0f && viskores::MagnitudeSquared(normal) < tol2)
  {
    auto inf = viskores::Infinity<CoordType>();
    Normal = viskores::Vec<CoordType, 3>(inf, inf, inf);
  }
}

template <typename CoordType>
VISKORES_EXEC_CONT CoordType Plane<CoordType>::DistanceTo(const Vector& point) const
{
  auto dist = viskores::Dot(point - this->Origin, this->Normal);
  return dist;
}

template <typename CoordType>
VISKORES_EXEC_CONT typename Plane<CoordType>::Vector Plane<CoordType>::ClosestPoint(
  const Vector& point) const
{
  auto vop = viskores::Project(point - this->Origin, this->Normal);
  auto closest = point - vop;
  return closest;
}

template <typename CoordType>
template <bool IsTwoSided>
VISKORES_EXEC_CONT bool Plane<CoordType>::Intersect(const Ray<CoordType, 3, IsTwoSided>& ray,
                                                    CoordType& parameter,
                                                    Vector& point,
                                                    bool& lineInPlane,
                                                    CoordType tol) const
{
  CoordType d0 = this->DistanceTo(ray.Origin);
  CoordType dirDot = viskores::Dot(this->Normal, ray.Direction);
  // If the ray/line lies parallel to the plane, the intersection is degenerate:
  if (viskores::Abs(dirDot) < tol)
  {
    lineInPlane = (viskores::Abs(d0) < tol);
    return false;
  }
  lineInPlane = false;
  parameter = -d0 / dirDot;
  // If we have a ray (not a line) and it points away from the
  // side of the plane where its origin lies, then there is no
  // intersection.
  if (!IsTwoSided)
  {
    if (parameter < 0.0f)
    {
      return false;
    }
  }

  // Check whether an endpoint lies in the plane:
  if (viskores::Abs(d0) < tol)
  {
    parameter = static_cast<CoordType>(0.0f);
    point = ray.Origin;
    return true;
  }

  // The perpendicular distance of the origin to the plane
  // forms one side of a triangle whose hypotenuse is the
  // parameter value (because ray.Direction has unit length).
  // The dot product of the plane normal and ray direction
  // is the cosine of the angle between the hypotenuse and
  // the shortest path to the plane, so....
  point = ray.Origin + parameter * ray.Direction;
  return true;
}

template <typename CoordType>
VISKORES_EXEC_CONT bool Plane<CoordType>::Intersect(const LineSegment<CoordType>& segment,
                                                    CoordType& parameter,
                                                    bool& lineInPlane) const
{
  Vector point;
  return this->Intersect(segment, parameter, point, lineInPlane);
}

template <typename CoordType>
VISKORES_EXEC_CONT bool Plane<CoordType>::Intersect(const LineSegment<CoordType>& segment,
                                                    CoordType& parameter,
                                                    Vector& point,
                                                    bool& lineInPlane) const
{
  CoordType d0 = this->DistanceTo(segment.Endpoints[0]);
  CoordType d1 = this->DistanceTo(segment.Endpoints[1]);
  if (d0 == 0 && d1 == 0)
  {
    // The entire segment lies in the plane.
    lineInPlane = true;
    return true;
  }

  lineInPlane = false;
  // Check whether an endpoint lies in the plane:
  if (d0 == 0)
  {
    parameter = static_cast<CoordType>(0.0f);
    point = segment.Endpoints[0];
    return true;
  }
  if (d1 == 0)
  {
    parameter = static_cast<CoordType>(1.0f);
    point = segment.Endpoints[1];
    return true;
  }

  // See whether endpoints lie on opposite sides of the plane:
  //
  // Note that the viskores::SignBit comments below cause an internal compiler
  // error on cuda 9.1, ubuntu 17.10 or they would be used:
  bool c0 = d0 < 0; // !!viskores::SignBit(d0);
  bool c1 = d1 < 0; // !!viskores::SignBit(d1);
  CoordType a0 = viskores::Abs(d0);
  CoordType a1 = viskores::Abs(d1);
  if (c0 == c1)
  {
    // Both endpoints lie to the same side of the plane, so
    // there is no intersection. Report the closest endpoint.
    parameter = static_cast<CoordType>(a0 < a1 ? 0.0f : 1.0f);
    point = segment.Endpoints[a0 < a1 ? 0 : 1];
    return false;
  }

  // Endpoint distances have the opposite sign; there must be
  // an intersection. It must occur at distance 0, and distance
  // varies linearly from d0 to d1, so...
  parameter = a0 / (a0 + a1);
  point = segment.Endpoints[0] * (1.0f - parameter) + segment.Endpoints[1] * parameter;
  return true;
}

template <typename CoordType>
VISKORES_EXEC_CONT bool Plane<CoordType>::Intersect(const Plane<CoordType>& other,
                                                    Ray<CoordType, 3, true>& ray,
                                                    bool& coincident,
                                                    CoordType tol2) const
{
  auto dir = viskores::Cross(this->Normal, other.Normal);
  auto mag2 = viskores::MagnitudeSquared(dir);
  if (mag2 < tol2)
  { // The planes are parallel.
    auto dist = this->DistanceTo(other.Origin);
    coincident = dist * dist < tol2;
    return false;
  }
  // The planes intersect. We want to find a point on the new plane
  // and we want it to be near the other plane base points to avoid
  // precision issues in the future.
  // So, project each plane origin to the other plane along a line
  // perpendicular to the plane and the output line. Both of these
  // points are on the output line. Average the two points. The result
  // will still be on the line and will be closer to the two base points.
  auto nn = viskores::Normal(dir);
  auto moveDir01 = viskores::Cross(this->Normal, nn);
  auto moveDir02 = viskores::Cross(other.Normal, nn);
  Ray<CoordType, 3, true> bra(this->Origin, moveDir01);
  Ray<CoordType, 3, true> brb(other.Origin, moveDir02);
  viskores::Vec<CoordType, 3> p0a;
  viskores::Vec<CoordType, 3> p0b;
  CoordType pDummyA, pDummyB;
  bool bDummyA, bDummyB;
  auto tol = viskores::Sqrt(tol2);
  this->Intersect(brb, pDummyA, p0a, bDummyA, tol);
  other.Intersect(bra, pDummyB, p0b, bDummyB, tol);
  ray = viskores::Ray<CoordType, 3, true>((p0a + p0b) * 0.5f, nn);
  return true;
}

// -----------------------------------------------------------------------------
// Sphere

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT Sphere<CoordType, Dim>::Sphere()
  : Center{ 0.f }
  , Radius(static_cast<CoordType>(1.f))
{
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT Sphere<CoordType, Dim>::Sphere(const Vector& center, CoordType radius)
  : Center(center)
  , Radius(radius <= 0.f ? static_cast<CoordType>(-1.0f) : radius)
{
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT bool Sphere<CoordType, Dim>::Contains(const Vector& point, CoordType tol2) const
{
  return this->Classify(point, tol2) < 0;
}

template <typename CoordType, int Dim>
VISKORES_EXEC_CONT int Sphere<CoordType, Dim>::Classify(const Vector& point, CoordType tol2) const
{
  if (!this->IsValid())
  {
    return 1; // All points are outside invalid spheres.
  }
  auto d2 = viskores::MagnitudeSquared(point - this->Center);
  auto r2 = this->Radius * this->Radius;
  return d2 < r2 - tol2 ? -1 : (d2 > r2 + tol2 ? +1 : 0);
}

// -----------------------------------------------------------------------------
// Construction techniques

template <typename CoordType, bool IsTwoSided>
VISKORES_EXEC_CONT viskores::Plane<CoordType> make_PlaneFromPointAndLine(
  const viskores::Vec<CoordType, 3>& point,
  const viskores::Ray<CoordType, 3, IsTwoSided>& ray,
  CoordType tol2)
{
  auto tmpDir = point - ray.Origin;
  return viskores::Plane<CoordType>(point, viskores::Cross(ray.Direction, tmpDir), tol2);
}

template <typename CoordType>
VISKORES_EXEC_CONT viskores::Plane<CoordType> make_PlaneFromPointAndLineSegment(
  const viskores::Vec<CoordType, 3>& point,
  const viskores::LineSegment3<CoordType>& segment,
  CoordType tol2)
{
  auto tmpDir = point - segment.Endpoints[0];
  return viskores::Plane<CoordType>(point, viskores::Cross(segment.Direction(), tmpDir), tol2);
}

template <typename CoordType>
VISKORES_EXEC_CONT viskores::Circle<CoordType> make_CircleFrom3Points(
  const typename viskores::Vec<CoordType, 2>& p0,
  const typename viskores::Vec<CoordType, 2>& p1,
  const typename viskores::Vec<CoordType, 2>& p2,
  CoordType tol)
{
  constexpr int Dim = 2;
  using Vector = typename viskores::Circle<CoordType>::Vector;

  viskores::LineSegment<CoordType, Dim> l01(p0, p1);
  viskores::LineSegment<CoordType, Dim> l02(p0, p2);
  auto pb01 = l01.PerpendicularBisector();
  auto pb02 = l02.PerpendicularBisector();
  Vector center;
  CoordType radius;
  if (!pb01.IsValid() || !pb02.IsValid())
  {
    center = Vector(0.f, 0.f);
    radius = -1.f;
    return viskores::Circle<CoordType>(center, radius);
  }
  auto isValid = pb01.Intersect(pb02, center, tol);
  radius = isValid ? viskores::Magnitude(center - p0) : static_cast<CoordType>(-1.0f);
  if (!isValid)
  { // Initialize center to something.
    center = Vector(viskores::Nan<CoordType>(), viskores::Nan<CoordType>());
  }
  return viskores::Circle<CoordType>(center, radius);
}

template <typename CoordType>
VISKORES_EXEC_CONT viskores::Sphere<CoordType, 3> make_SphereFrom4Points(
  const viskores::Vec<CoordType, 3>& a0,
  const viskores::Vec<CoordType, 3>& a1,
  const viskores::Vec<CoordType, 3>& a2,
  const viskores::Vec<CoordType, 3>& a3,
  CoordType tol)
{
  // Choose p3 such that the min(p3 - p[012]) is larger than any other choice of p3.
  // From: http://steve.hollasch.net/cgindex/geometry/sphere4pts.html,
  // retrieved 2018-06-12 and paraphrased in terms of local variable names:
  //
  // If circlePointInPlaneOfP3-p3 is much smaller than
  // circlePointInPlaneOfP3-circleCenterWorld, then the
  // sphere center will be very close to circleCenterWorld
  // and subject to error.
  // It's best to choose p3 so that the least of p0-p3, p1-p3, and
  // p2-p3 is larger than for any other.

  CoordType d0 = viskores::MagnitudeSquared(a1 - a0);
  CoordType d1 = viskores::MagnitudeSquared(a2 - a0);
  CoordType d2 = viskores::MagnitudeSquared(a3 - a0);
  CoordType d3 = viskores::MagnitudeSquared(a2 - a1);
  CoordType d4 = viskores::MagnitudeSquared(a3 - a1);
  CoordType d5 = viskores::MagnitudeSquared(a3 - a2);
  CoordType sel0 = viskores::Min(d0, viskores::Min(d1, d2));
  CoordType sel1 = viskores::Min(d0, viskores::Min(d3, d4));
  CoordType sel2 = viskores::Min(d1, viskores::Min(d3, d5));
  CoordType sel3 = viskores::Min(d2, viskores::Min(d4, d5));
  CoordType selm = viskores::Max(viskores::Max(sel0, sel1), viskores::Max(sel2, sel3));

  viskores::Vec<CoordType, 3> p0 = a0;
  viskores::Vec<CoordType, 3> p1 = a1;
  viskores::Vec<CoordType, 3> p2 = a2;
  viskores::Vec<CoordType, 3> p3 = a3;
  if (sel0 == selm)
  {
    p3 = a0;
    p0 = a3;
  }
  else if (sel1 == selm)
  {
    p3 = a1;
    p1 = a3;
  }
  else if (sel2 == selm)
  {
    p3 = a2;
    p2 = a3;
  }
  else // sel3 == selm
  {
    // do nothing.
  }

  viskores::Vec<viskores::Vec<CoordType, 3>, 3> axes;
  axes[1] = p1 - p0;
  axes[2] = p2 - p0;
  axes[0] = viskores::Cross(axes[1], axes[2]);
  viskores::Vec<viskores::Vec<CoordType, 3>, 3> basis;
  int rank = viskores::Orthonormalize(axes, basis, tol);
  if (rank < 3)
  {
    viskores::Sphere<CoordType, 3> invalid;
    invalid.Radius = -1.0f;
    return invalid;
  }

  // Project points to plane and fit a circle.
  auto p0_p = viskores::Vec<CoordType, 2>{ 0.f }; // This is p0's new coordinate...
  auto p1_p = viskores::Vec<CoordType, 2>(viskores::ProjectedDistance(axes[1], basis[1]),
                                          viskores::ProjectedDistance(axes[1], basis[2]));
  auto p2_p = viskores::Vec<CoordType, 2>(viskores::ProjectedDistance(axes[2], basis[1]),
                                          viskores::ProjectedDistance(axes[2], basis[2]));

  auto circle = make_CircleFrom3Points(p0_p, p1_p, p2_p);
  if (!circle.IsValid())
  {
    viskores::Sphere<CoordType, 3> invalid;
    invalid.Radius = -1.0f;
    return invalid;
  }

  viskores::Vec<CoordType, 3> circleCenterWorld =
    p0 + circle.Center[0] * basis[1] + circle.Center[1] * basis[2];

  viskores::Line3<CoordType> centerRay(circleCenterWorld, basis[0]);
  // If our remaining unused point (p3) lines on centerRay,
  // use one of the other points to locate the sphere's center:
  viskores::Vec<CoordType, 3> circlePointInPlaneOfP3;
  if (viskores::Abs(centerRay.DistanceTo(p3)) < tol)
  {
    circlePointInPlaneOfP3 = p0;
  }
  else
  {
    Plane<CoordType> pp3(circleCenterWorld, basis[0]);
    circlePointInPlaneOfP3 = circleCenterWorld +
      viskores::Normal(pp3.ClosestPoint(p3) - circleCenterWorld) * circle.Radius;
  }

  auto bisectorPlane =
    viskores::LineSegment3<CoordType>(circlePointInPlaneOfP3, p3).PerpendicularBisector();
  viskores::Vec<CoordType, 3> sphereCenter;
  CoordType param;
  bool lineInPlane;
  if (!bisectorPlane.Intersect(centerRay, param, sphereCenter, lineInPlane, tol))
  {
    viskores::Sphere<CoordType, 3> invalid;
    invalid.Radius = -1.0f;
    return invalid;
  }
  auto sphereRadius = viskores::Magnitude(sphereCenter - p3);
  viskores::Sphere<CoordType, 3> result(sphereCenter, sphereRadius);
  ;
  return result;
}

} // namespace viskores
