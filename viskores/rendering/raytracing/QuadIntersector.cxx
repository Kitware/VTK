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

#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/rendering/raytracing/BVHTraverser.h>
#include <viskores/rendering/raytracing/QuadIntersector.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

#define QUAD_AABB_EPSILON 1.0e-4f
class FindQuadAABBs : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  FindQuadAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  VISKORES_EXEC void operator()(const viskores::Vec<viskores::Id, 5> quadId,
                                viskores::Float32& xmin,
                                viskores::Float32& ymin,
                                viskores::Float32& zmin,
                                viskores::Float32& xmax,
                                viskores::Float32& ymax,
                                viskores::Float32& zmax,
                                const PointPortalType& points) const
  {
    // cast to Float32
    viskores::Vec3f_32 q, r, s, t;

    q = static_cast<viskores::Vec3f_32>(points.Get(quadId[1]));
    r = static_cast<viskores::Vec3f_32>(points.Get(quadId[2]));
    s = static_cast<viskores::Vec3f_32>(points.Get(quadId[3]));
    t = static_cast<viskores::Vec3f_32>(points.Get(quadId[4]));

    xmin = q[0];
    ymin = q[1];
    zmin = q[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    xmin = viskores::Min(xmin, r[0]);
    ymin = viskores::Min(ymin, r[1]);
    zmin = viskores::Min(zmin, r[2]);
    xmax = viskores::Max(xmax, r[0]);
    ymax = viskores::Max(ymax, r[1]);
    zmax = viskores::Max(zmax, r[2]);
    xmin = viskores::Min(xmin, s[0]);
    ymin = viskores::Min(ymin, s[1]);
    zmin = viskores::Min(zmin, s[2]);
    xmax = viskores::Max(xmax, s[0]);
    ymax = viskores::Max(ymax, s[1]);
    zmax = viskores::Max(zmax, s[2]);
    xmin = viskores::Min(xmin, t[0]);
    ymin = viskores::Min(ymin, t[1]);
    zmin = viskores::Min(zmin, t[2]);
    xmax = viskores::Max(xmax, t[0]);
    ymax = viskores::Max(ymax, t[1]);
    zmax = viskores::Max(zmax, t[2]);

    viskores::Float32 xEpsilon, yEpsilon, zEpsilon;
    const viskores::Float32 minEpsilon = 1e-6f;
    xEpsilon = viskores::Max(minEpsilon, QUAD_AABB_EPSILON * (xmax - xmin));
    yEpsilon = viskores::Max(minEpsilon, QUAD_AABB_EPSILON * (ymax - ymin));
    zEpsilon = viskores::Max(minEpsilon, QUAD_AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }

}; //class FindAABBs

template <typename Device>
class QuadLeafIntersector
{
public:
  using IdType = viskores::Vec<viskores::Id, 5>;
  using IdHandle = viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  IdArrayPortal QuadIds;

  QuadLeafIntersector() {}

  QuadLeafIntersector(const IdHandle& quadIds, viskores::cont::Token& token)
    : QuadIds(quadIds.PrepareForInput(Device(), token))
  {
  }

  template <typename vec3, typename Precision>
  VISKORES_EXEC bool quad(const vec3& ray_origin,
                          const vec3& ray_direction,
                          const vec3& v00,
                          const vec3& v10,
                          const vec3& v11,
                          const vec3& v01,
                          Precision& u,
                          Precision& v,
                          Precision& t) const
  {

    /* An Eﬃcient Ray-Quadrilateral Intersection Test
         Ares Lagae Philip Dutr´e
         http://graphics.cs.kuleuven.be/publications/LD05ERQIT/index.html

      v01 *------------ * v11
          |\           |
          |  \         |
          |    \       |
          |      \     |
          |        \   |
          |          \ |
      v00 *------------* v10
      */
    // Rejects rays that are parallel to Q, and rays that intersect the plane of
    // Q either on the left of the line V00V01 or on the right of the line V00V10.

    vec3 E03 = v01 - v00;
    vec3 P = viskores::Cross(ray_direction, E03);
    vec3 E01 = v10 - v00;
    Precision det = viskores::dot(E01, P);

    if (viskores::Abs(det) < viskores::Epsilon<Precision>())
      return false;
    Precision inv_det = 1.0f / det;
    vec3 T = ray_origin - v00;
    Precision alpha = viskores::dot(T, P) * inv_det;
    if (alpha < 0.0)
      return false;
    vec3 Q = viskores::Cross(T, E01);
    Precision beta = viskores::dot(ray_direction, Q) * inv_det;
    if (beta < 0.0)
      return false;

    if ((alpha + beta) > 1.0f)
    {

      // Rejects rays that intersect the plane of Q either on the
      // left of the line V11V10 or on the right of the line V11V01.

      vec3 E23 = v01 - v11;
      vec3 E21 = v10 - v11;
      vec3 P_prime = viskores::Cross(ray_direction, E21);
      Precision det_prime = viskores::dot(E23, P_prime);
      if (viskores::Abs(det_prime) < viskores::Epsilon<Precision>())
        return false;
      Precision inv_det_prime = 1.0f / det_prime;
      vec3 T_prime = ray_origin - v11;
      Precision alpha_prime = viskores::dot(T_prime, P_prime) * inv_det_prime;
      if (alpha_prime < 0.0f)
        return false;
      vec3 Q_prime = viskores::Cross(T_prime, E23);
      Precision beta_prime = viskores::dot(ray_direction, Q_prime) * inv_det_prime;
      if (beta_prime < 0.0f)
        return false;
    }

    // Compute the ray parameter of the intersection point, and
    // reject the ray if it does not hit Q.

    t = viskores::dot(E03, Q) * inv_det;
    if (t < 0.0)
      return false;


    // Compute the barycentric coordinates of V11
    Precision alpha_11, beta_11;
    vec3 E02 = v11 - v00;
    vec3 n = viskores::Cross(E01, E02);

    if ((viskores::Abs(n[0]) >= viskores::Abs(n[1])) &&
        (viskores::Abs(n[0]) >= viskores::Abs(n[2])))
    {

      alpha_11 = ((E02[1] * E03[2]) - (E02[2] * E03[1])) / n[0];
      beta_11 = ((E01[1] * E02[2]) - (E01[2] * E02[1])) / n[0];
    }
    else if ((viskores::Abs(n[1]) >= viskores::Abs(n[0])) &&
             (viskores::Abs(n[1]) >= viskores::Abs(n[2])))
    {

      alpha_11 = ((E02[2] * E03[0]) - (E02[0] * E03[2])) / n[1];
      beta_11 = ((E01[2] * E02[0]) - (E01[0] * E02[2])) / n[1];
    }
    else
    {

      alpha_11 = ((E02[0] * E03[1]) - (E02[1] * E03[0])) / n[2];
      beta_11 = ((E01[0] * E02[1]) - (E01[1] * E02[0])) / n[2];
    }

    // Compute the bilinear coordinates of the intersection point.
    if (viskores::Abs(alpha_11 - 1.0f) < viskores::Epsilon<Precision>())
    {

      u = alpha;
      if (viskores::Abs(beta_11 - 1.0f) < viskores::Epsilon<Precision>())
        v = beta;
      else
        v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }
    else if (viskores::Abs(beta_11 - 1.0) < viskores::Epsilon<Precision>())
    {

      v = beta;
      u = alpha / ((v * (alpha_11 - 1.0f)) + 1.0f);
    }
    else
    {

      Precision A = 1.0f - beta_11;
      Precision B = (alpha * (beta_11 - 1.0f)) - (beta * (alpha_11 - 1.0f)) - 1.0f;
      Precision C = alpha;
      Precision D = (B * B) - (4.0f * A * C);
      Precision QQ = -0.5f * (B + ((B < 0.0f ? -1.0f : 1.0f) * viskores::Sqrt(D)));
      u = QQ / A;
      if ((u < 0.0f) || (u > 1.0f))
        u = C / QQ;
      v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }

    return true;
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VISKORES_EXEC inline void IntersectLeaf(
    const viskores::Int32& currentNode,
    const viskores::Vec<Precision, 3>& origin,
    const viskores::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    viskores::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& minU,
    Precision& minV,
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const viskores::Id quadCount = leafs.Get(currentNode);
    for (viskores::Id i = 1; i <= quadCount; ++i)
    {
      const viskores::Id quadIndex = leafs.Get(currentNode + i);
      if (quadIndex < QuadIds.GetNumberOfValues())
      {
        IdType pointIndex = QuadIds.Get(quadIndex);
        Precision dist;
        viskores::Vec<Precision, 3> q, r, s, t;
        q = viskores::Vec<Precision, 3>(points.Get(pointIndex[1]));
        r = viskores::Vec<Precision, 3>(points.Get(pointIndex[2]));
        s = viskores::Vec<Precision, 3>(points.Get(pointIndex[3]));
        t = viskores::Vec<Precision, 3>(points.Get(pointIndex[4]));
        Precision u, v;

        bool ret = quad(origin, dir, q, r, s, t, u, v, dist);
        if (ret)
        {
          if (dist < closestDistance && dist > minDistance)
          {
            //matid = viskores::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = dist;
            hitIndex = quadIndex;
            minU = u;
            minV = v;
          }
        }
      }
    } // for
  }
};

class QuadExecWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using IdType = viskores::Vec<viskores::Id, 5>;
  using IdHandle = viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>>;
  IdHandle QuadIds;

public:
  QuadExecWrapper(IdHandle& quadIds)
    : QuadIds(quadIds)
  {
  }

  template <typename Device>
  VISKORES_CONT QuadLeafIntersector<Device> PrepareForExecution(Device,
                                                                viskores::cont::Token& token) const
  {
    return QuadLeafIntersector<Device>(QuadIds, token);
  }
};

class CalculateNormals : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CalculateNormals() {}
  typedef void
    ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
  template <typename Precision, typename PointPortalType, typename IndicesPortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id& hitIndex,
                                       const viskores::Vec<Precision, 3>& rayDir,
                                       Precision& normalX,
                                       Precision& normalY,
                                       Precision& normalZ,
                                       const PointPortalType& points,
                                       const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Vec<viskores::Id, 5> quadId = indicesPortal.Get(hitIndex);

    viskores::Vec<Precision, 3> a, b, c;
    a = points.Get(quadId[1]);
    b = points.Get(quadId[2]);
    c = points.Get(quadId[3]);

    viskores::Vec<Precision, 3> normal = viskores::TriangleNormal(a, b, c);
    viskores::Normalize(normal);

    //flip the normal if its pointing the wrong way
    if (viskores::dot(normal, rayDir) > 0.f)
      normal = -normal;

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }
}; //class CalculateNormals

template <typename Precision>
class GetLerpedScalar : public viskores::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Normalize;

public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  VISKORES_CONT
  GetLerpedScalar(const viskores::Float32& minScalar, const viskores::Float32& maxScalar)
    : MinScalar(minScalar)
  {
    Normalize = true;
    if (minScalar >= maxScalar)
    {
      // support the scalar renderer
      Normalize = false;
      this->InvDeltaScalar = Precision(0.f);
    }
    else
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      this->InvDeltaScalar = 1.f / (maxScalar - MinScalar);
    }
  }
  template <typename ScalarPortalType, typename IndicesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                const Precision& u,
                                const Precision& v,
                                Precision& scalar,
                                const ScalarPortalType& scalars,
                                const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Vec<viskores::Id, 5> pointId = indicesPortal.Get(hitIndex);

    Precision aScalar = Precision(scalars.Get(pointId[1]));
    Precision bScalar = Precision(scalars.Get(pointId[2]));
    Precision cScalar = Precision(scalars.Get(pointId[3]));
    Precision dScalar = Precision(scalars.Get(pointId[4]));

    Precision uP = 1.0f - u;
    Precision vP = 1.0f - v;
    scalar = uP * vP * aScalar + u * vP * bScalar + u * v * cScalar + uP * v * dScalar;

    if (Normalize)
    {
      scalar = (scalar - MinScalar) * this->InvDeltaScalar;
    }
  }
}; //class GetLerpedScalar

template <typename Precision>
class GetNodalScalar : public viskores::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Normalize;

public:
  using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4);

  VISKORES_CONT
  GetNodalScalar(const viskores::Float32& minScalar, const viskores::Float32& maxScalar)
    : MinScalar(minScalar)
  {
    Normalize = true;
    if (minScalar >= maxScalar)
    {
      // support the scalar renderer
      Normalize = false;
      this->InvDeltaScalar = Precision(0.f);
    }
    else
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      this->InvDeltaScalar = 1.f / (maxScalar - MinScalar);
    }
  }

  template <typename ScalarPortalType, typename IndicesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                Precision& scalar,
                                const ScalarPortalType& scalars,
                                const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Vec<viskores::Id, 5> pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    if (Normalize)
    {
      scalar = (scalar - MinScalar) * this->InvDeltaScalar;
    }
  }
}; //class GetNodalScalar

} // namespace detail

QuadIntersector::QuadIntersector()
  : ShapeIntersector()
{
}

QuadIntersector::~QuadIntersector() {}


void QuadIntersector::IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void QuadIntersector::IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void QuadIntersector::IntersectRaysImp(Ray<Precision>& rays, bool viskoresNotUsed(returnCellIndex))
{

  detail::QuadExecWrapper leafIntersector(this->QuadIds);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void QuadIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                          const viskores::cont::Field scalarField,
                                          const viskores::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  // TODO: if this is nodes of a mesh, support points
  const bool isSupportedField = scalarField.IsCellField() || scalarField.IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue("Field not accociated with a cell set");
  }

  viskores::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, CoordsHandle, QuadIds);

  if (scalarField.IsPointField())
  {
    viskores::worklet::DispatcherMapField<detail::GetLerpedScalar<Precision>>(
      detail::GetLerpedScalar<Precision>(viskores::Float32(scalarRange.Min),
                                         viskores::Float32(scalarRange.Max)))
      .Invoke(rays.HitIdx,
              rays.U,
              rays.V,
              rays.Scalar,
              viskores::rendering::raytracing::GetScalarFieldArray(scalarField),
              QuadIds);
  }
  else
  {
    viskores::worklet::DispatcherMapField<detail::GetNodalScalar<Precision>>(
      detail::GetNodalScalar<Precision>(viskores::Float32(scalarRange.Min),
                                        viskores::Float32(scalarRange.Max)))
      .Invoke(rays.HitIdx,
              rays.Scalar,
              viskores::rendering::raytracing::GetScalarFieldArray(scalarField),
              QuadIds);
  }
}

void QuadIntersector::IntersectionData(Ray<viskores::Float32>& rays,
                                       const viskores::cont::Field scalarField,
                                       const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::IntersectionData(Ray<viskores::Float64>& rays,
                                       const viskores::cont::Field scalarField,
                                       const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                              viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> quadIds)
{

  this->QuadIds = quadIds;
  this->CoordsHandle = coords;
  AABBs AABB;

  viskores::worklet::DispatcherMapField<detail::FindQuadAABBs> faabbsInvoker;
  faabbsInvoker.Invoke(this->QuadIds,
                       AABB.xmins,
                       AABB.ymins,
                       AABB.zmins,
                       AABB.xmaxs,
                       AABB.ymaxs,
                       AABB.zmaxs,
                       CoordsHandle);

  this->SetAABBs(AABB);
}

viskores::Id QuadIntersector::GetNumberOfShapes() const
{
  return QuadIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
