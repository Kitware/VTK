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
#include <viskores/rendering/raytracing/CylinderIntersector.h>
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

class FindCylinderAABBs : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  FindCylinderAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  template <typename PointPortalType>
  VISKORES_EXEC void operator()(const viskores::Id3 cylId,
                                const viskores::Float32& radius,
                                viskores::Float32& xmin,
                                viskores::Float32& ymin,
                                viskores::Float32& zmin,
                                viskores::Float32& xmax,
                                viskores::Float32& ymax,
                                viskores::Float32& zmax,
                                const PointPortalType& points) const
  {
    // cast to Float32
    viskores::Vec3f_32 point1, point2;
    viskores::Vec3f_32 temp;

    point1 = static_cast<viskores::Vec3f_32>(points.Get(cylId[1]));
    point2 = static_cast<viskores::Vec3f_32>(points.Get(cylId[2]));

    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
    xmin = ymin = zmin = viskores::Infinity32();
    xmax = ymax = zmax = viskores::NegativeInfinity32();


    //set first point to max and min
    Bounds(point1, radius, xmin, ymin, zmin, xmax, ymax, zmax);

    Bounds(point2, radius, xmin, ymin, zmin, xmax, ymax, zmax);
  }

  VISKORES_EXEC void Bounds(const viskores::Vec3f_32& point,
                            const viskores::Float32& radius,
                            viskores::Float32& xmin,
                            viskores::Float32& ymin,
                            viskores::Float32& zmin,
                            viskores::Float32& xmax,
                            viskores::Float32& ymax,
                            viskores::Float32& zmax) const
  {
    viskores::Vec3f_32 temp, p;
    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
    p = point + temp;

    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);

    p = point - temp;
    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = radius;
    temp[2] = 0.f;

    p = point + temp;
    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);

    p = point - temp;
    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = 0.f;
    temp[2] = radius;

    p = point + temp;
    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);

    p = point - temp;
    xmin = viskores::Min(xmin, p[0]);
    xmax = viskores::Max(xmax, p[0]);
    ymin = viskores::Min(ymin, p[1]);
    ymax = viskores::Max(ymax, p[1]);
    zmin = viskores::Min(zmin, p[2]);
    zmax = viskores::Max(zmax, p[2]);
  }
}; //class FindAABBs

template <typename Device>
class CylinderLeafIntersector
{
public:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id3>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  using FloatPortal = typename FloatHandle::ReadPortalType;
  using UInt8Handle = viskores::cont::ArrayHandle<viskores::UInt8>;
  using UInt8Portal = typename UInt8Handle::ReadPortalType;
  IdArrayPortal CylIds;
  FloatPortal Radii;
  UInt8Portal CapMasks;
  bool UseCapMasks;

  CylinderLeafIntersector()
    : UseCapMasks(false)
  {
  }

  CylinderLeafIntersector(const IdHandle& cylIds,
                          const FloatHandle& radii,
                          const UInt8Handle& capMasks,
                          bool useCapMasks,
                          viskores::cont::Token& token)
    : CylIds(cylIds.PrepareForInput(Device(), token))
    , Radii(radii.PrepareForInput(Device(), token))
    , CapMasks(capMasks.PrepareForInput(Device(), token))
    , UseCapMasks(useCapMasks)
  {
  }

  template <typename vec3>
  VISKORES_EXEC vec3 cylinder(const vec3& ray_start,
                              const vec3& ray_direction,
                              const vec3& p,
                              const vec3& q,
                              float r) const
  {
    float t = 0;
    vec3 d = q - p;
    vec3 m = ray_start - p;

    vec3 s = ray_start - q;

    viskores::Float32 mdotm = viskores::Float32(viskores::dot(m, m));
    vec3 n = ray_direction *
      (viskores::Max(mdotm, static_cast<viskores::Float32>(viskores::dot(s, s))) + r);

    viskores::Float32 mdotd = viskores::Float32(viskores::dot(m, d));
    viskores::Float32 ndotd = viskores::Float32(viskores::dot(n, d));
    viskores::Float32 ddotd = viskores::Float32(viskores::dot(d, d));
    if ((mdotd < 0.0f) && (mdotd + ndotd < 0.0f))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    if ((mdotd > ddotd) && (mdotd + ndotd > ddotd))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    viskores::Float32 ndotn = viskores::Float32(viskores::dot(n, n));
    viskores::Float32 nlen = viskores::Float32(sqrt(ndotn));
    viskores::Float32 mdotn = viskores::Float32(viskores::dot(m, n));
    viskores::Float32 a = ddotd * ndotn - ndotd * ndotd;
    viskores::Float32 k = mdotm - r * r;
    viskores::Float32 c = ddotd * k - mdotd * mdotd;

    if (fabs(a) < 1e-6)
    {
      if (c > 0.0)
      {
        return vec3(0, 0, 0);
      }
      if (mdotd < 0.0f)
      {
        t = -mdotn / ndotn;
      }
      else if (mdotd > ddotd)
      {
        t = (ndotd - mdotn) / ndotn;
      }
      else
        t = 0;

      return vec3(1, t * nlen, 0);
    }
    viskores::Float32 b = ddotd * mdotn - ndotd * mdotd;
    viskores::Float32 discr = b * b - a * c;
    if (discr < 0.0f)
    {
      return vec3(0, 0, 0);
    }
    t = (-b - viskores::Sqrt(discr)) / a;
    if (t < 0.0f || t > 1.0f)
    {
      return vec3(0, 0, 0);
    }

    viskores::Float32 u = mdotd + t * ndotd;

    if (u > ddotd)
    {
      if (ndotd >= 0.0f)
      {
        return vec3(0, 0, 0);
      }
      t = (ddotd - mdotd) / ndotd;

      return vec3(
        k + ddotd - 2 * mdotd + t * (2 * (mdotn - ndotd) + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    else if (u < 0.0f)
    {
      if (ndotd <= 0.0f)
      {
        return vec3(0.0, 0.0, 0);
      }
      t = -mdotd / ndotd;

      return vec3(k + 2 * t * (mdotn + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    return vec3(1, t * nlen, 0);
  }

  template <typename Precision>
  VISKORES_EXEC void CheckHit(const Precision& t,
                              const Precision& minDistance,
                              Precision& closestDistance,
                              Precision& hitType,
                              const Precision& candidateType) const
  {
    if (t < closestDistance && t > minDistance)
    {
      closestDistance = t;
      hitType = candidateType;
    }
  }

  template <typename vec3, typename Precision>
  VISKORES_EXEC bool cappedCylinder(const vec3& rayStart,
                                    const vec3& rayDirection,
                                    const vec3& first,
                                    const vec3& second,
                                    const Precision& radius,
                                    const viskores::UInt8 capMask,
                                    const Precision& minDistance,
                                    Precision& closestDistance,
                                    Precision& hitType) const
  {
    constexpr Precision epsilon = Precision(1e-6f);

    const vec3 axis = second - first;
    const vec3 relOrigin = rayStart - first;
    const Precision axisLength2 = viskores::dot(axis, axis);
    if (axisLength2 <= epsilon)
    {
      return false;
    }

    const Precision rayAxis = viskores::dot(rayDirection, axis);
    const Precision originAxis = viskores::dot(relOrigin, axis);
    const Precision rayLength2 = viskores::dot(rayDirection, rayDirection);
    const Precision originLength2 = viskores::dot(relOrigin, relOrigin);
    const Precision originRay = viskores::dot(relOrigin, rayDirection);
    const Precision radius2 = radius * radius;

    bool hit = false;
    const Precision originalClosest = closestDistance;

    const Precision a = axisLength2 * rayLength2 - rayAxis * rayAxis;
    const Precision b = axisLength2 * originRay - originAxis * rayAxis;
    const Precision c =
      axisLength2 * originLength2 - originAxis * originAxis - radius2 * axisLength2;

    if (viskores::Abs(a) > epsilon)
    {
      const Precision discriminant = b * b - a * c;
      if (discriminant >= Precision(0))
      {
        const Precision sqrtDiscriminant = viskores::Sqrt(discriminant);
        const Precision invA = Precision(1) / a;
        const Precision t0 = (-b - sqrtDiscriminant) * invA;
        const Precision z0 = originAxis + t0 * rayAxis;
        if (z0 >= Precision(0) && z0 <= axisLength2)
        {
          this->CheckHit(t0, minDistance, closestDistance, hitType, Precision(0));
        }

        const Precision t1 = (-b + sqrtDiscriminant) * invA;
        const Precision z1 = originAxis + t1 * rayAxis;
        if (z1 >= Precision(0) && z1 <= axisLength2)
        {
          this->CheckHit(t1, minDistance, closestDistance, hitType, Precision(0));
        }
      }
    }

    if (viskores::Abs(rayAxis) > epsilon)
    {
      if ((capMask & viskores::UInt8(1)) != 0)
      {
        const Precision t = -originAxis / rayAxis;
        const vec3 offset = relOrigin + rayDirection * t;
        if (viskores::dot(offset, offset) <= radius2)
        {
          this->CheckHit(t, minDistance, closestDistance, hitType, Precision(1));
        }
      }

      if ((capMask & viskores::UInt8(2)) != 0)
      {
        const Precision t = (axisLength2 - originAxis) / rayAxis;
        const vec3 offset = rayStart + rayDirection * t - second;
        if (viskores::dot(offset, offset) <= radius2)
        {
          this->CheckHit(t, minDistance, closestDistance, hitType, Precision(2));
        }
      }
    }

    hit = closestDistance < originalClosest;
    return hit;
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
    Precision& viskoresNotUsed(minV),
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const viskores::Id cylCount = leafs.Get(currentNode);
    for (viskores::Id i = 1; i <= cylCount; ++i)
    {
      const viskores::Id cylIndex = leafs.Get(currentNode + i);
      if (cylIndex < CylIds.GetNumberOfValues())
      {
        viskores::Id3 pointIndex = CylIds.Get(cylIndex);
        viskores::Float32 radius = Radii.Get(cylIndex);
        viskores::Vec<Precision, 3> bottom, top;
        bottom = viskores::Vec<Precision, 3>(points.Get(pointIndex[1]));
        top = viskores::Vec<Precision, 3>(points.Get(pointIndex[2]));

        if (this->UseCapMasks)
        {
          const viskores::UInt8 capMask = (cylIndex < this->CapMasks.GetNumberOfValues())
            ? this->CapMasks.Get(cylIndex)
            : viskores::UInt8(0);
          Precision hitType = Precision(0);
          if (this->cappedCylinder(origin,
                                   dir,
                                   bottom,
                                   top,
                                   Precision(radius),
                                   capMask,
                                   minDistance,
                                   closestDistance,
                                   hitType))
          {
            hitIndex = cylIndex;
            minU = hitType;
          }
        }
        else
        {
          viskores::Vec3f_32 ret;
          ret = cylinder(origin, dir, bottom, top, radius);
          if (ret[0] > 0)
          {
            if (ret[1] < closestDistance && ret[1] > minDistance)
            {
              //matid = viskores::Vec<, 3>(points.Get(cur_offset + 2))[0];
              closestDistance = ret[1];
              hitIndex = cylIndex;
              minU = Precision(0);
            }
          }
        }
      }
    } // for
  }
};

class CylinderLeafWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id3>;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  using UInt8Handle = viskores::cont::ArrayHandle<viskores::UInt8>;
  IdHandle CylIds;
  FloatHandle Radii;
  UInt8Handle CapMasks;
  bool UseCapMasks;

public:
  CylinderLeafWrapper(IdHandle& cylIds, FloatHandle radii, UInt8Handle capMasks, bool useCapMasks)
    : CylIds(cylIds)
    , Radii(radii)
    , CapMasks(capMasks)
    , UseCapMasks(useCapMasks)
  {
  }

  template <typename Device>
  VISKORES_CONT CylinderLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return CylinderLeafIntersector<Device>(
      this->CylIds, this->Radii, this->CapMasks, this->UseCapMasks, token);
  }
};

class CalculateNormals : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CalculateNormals() {}
  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename Precision, typename PointPortalType, typename IndicesPortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id& hitIndex,
                                       const Precision& hitType,
                                       const viskores::Vec<Precision, 3>& intersection,
                                       Precision& normalX,
                                       Precision& normalY,
                                       Precision& normalZ,
                                       const PointPortalType& points,
                                       const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Id3 cylId = indicesPortal.Get(hitIndex);

    viskores::Vec<Precision, 3> a, b;
    a = points.Get(cylId[1]);
    b = points.Get(cylId[2]);

    viskores::Vec<Precision, 3> ap, ab;
    ap = intersection - a;
    ab = b - a;

    if (hitType == Precision(1) || hitType == Precision(2))
    {
      viskores::Vec<Precision, 3> normal = ab;
      viskores::Normalize(normal);
      if (hitType == Precision(1))
      {
        normal = -normal;
      }

      normalX = normal[0];
      normalY = normal[1];
      normalZ = normal[2];
      return;
    }

    // Project onto the cylinder axis. The denominator is the squared axis length.
    Precision mag2 = viskores::dot(ab, ab);
    Precision len = viskores::dot(ab, ap);
    Precision t = len / mag2;

    viskores::Vec<Precision, 3> center;
    center = a + t * ab;

    viskores::Vec<Precision, 3> normal = intersection - center;
    viskores::Normalize(normal);

    //flip the normal if its pointing the wrong way
    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }
}; //class CalculateNormals

template <typename Precision>
class GetScalar : public viskores::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Normalize;

public:
  VISKORES_CONT
  GetScalar(const viskores::Float32& minScalar, const viskores::Float32& maxScalar)
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
      this->InvDeltaScalar = 1.f / (maxScalar - this->MinScalar);
    }
  }
  typedef void ControlSignature(FieldIn, FieldInOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename ScalarPortalType, typename IndicesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                Precision& scalar,
                                const ScalarPortalType& scalars,
                                const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    //TODO: this should be interpolated?
    viskores::Id3 pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    if (Normalize)
    {
      scalar = (scalar - this->MinScalar) * this->InvDeltaScalar;
    }
  }
}; //class GetScalar

} // namespace detail

CylinderIntersector::CylinderIntersector()
  : ShapeIntersector()
  , UseCapMasks(false)
{
}

CylinderIntersector::~CylinderIntersector() {}

void CylinderIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                                  viskores::cont::ArrayHandle<viskores::Id3> cylIds,
                                  viskores::cont::ArrayHandle<viskores::Float32> radii)
{
  this->UseCapMasks = false;
  this->SetData(coords, cylIds, radii, viskores::cont::ArrayHandle<viskores::UInt8>{});
}

void CylinderIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                                  viskores::cont::ArrayHandle<viskores::Id3> cylIds,
                                  viskores::cont::ArrayHandle<viskores::Float32> radii,
                                  viskores::cont::ArrayHandle<viskores::UInt8> capMasks)
{
  this->Radii = radii;
  this->CylIds = cylIds;
  this->CapMasks = capMasks;
  this->UseCapMasks = capMasks.GetNumberOfValues() > 0;
  this->CoordsHandle = coords;
  AABBs AABB;

  viskores::worklet::DispatcherMapField<detail::FindCylinderAABBs>(detail::FindCylinderAABBs())
    .Invoke(this->CylIds,
            this->Radii,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

void CylinderIntersector::IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void CylinderIntersector::IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void CylinderIntersector::IntersectRaysImp(Ray<Precision>& rays,
                                           bool viskoresNotUsed(returnCellIndex))
{

  detail::CylinderLeafWrapper leafIntersector(
    this->CylIds, this->Radii, this->CapMasks, this->UseCapMasks);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void CylinderIntersector::IntersectionDataImp(Ray<Precision>& rays,
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
    .Invoke(rays.HitIdx,
            rays.U,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            CylIds);

  viskores::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(viskores::Float32(scalarRange.Min),
                                 viskores::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            viskores::rendering::raytracing::GetScalarFieldArray(scalarField),
            CylIds);
}

void CylinderIntersector::IntersectionData(Ray<viskores::Float32>& rays,
                                           const viskores::cont::Field scalarField,
                                           const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void CylinderIntersector::IntersectionData(Ray<viskores::Float64>& rays,
                                           const viskores::cont::Field scalarField,
                                           const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

viskores::Id CylinderIntersector::GetNumberOfShapes() const
{
  return CylIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
