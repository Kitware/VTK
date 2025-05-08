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
  IdArrayPortal CylIds;
  FloatPortal Radii;

  CylinderLeafIntersector() {}

  CylinderLeafIntersector(const IdHandle& cylIds,
                          const FloatHandle& radii,
                          viskores::cont::Token& token)
    : CylIds(cylIds.PrepareForInput(Device(), token))
    , Radii(radii.PrepareForInput(Device(), token))
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

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VISKORES_EXEC inline void IntersectLeaf(
    const viskores::Int32& currentNode,
    const viskores::Vec<Precision, 3>& origin,
    const viskores::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    viskores::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& viskoresNotUsed(minU),
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

        viskores::Vec3f_32 ret;
        ret = cylinder(origin, dir, bottom, top, radius);
        if (ret[0] > 0)
        {
          if (ret[1] < closestDistance && ret[1] > minDistance)
          {
            //matid = viskores::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = ret[1];
            hitIndex = cylIndex;
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
  IdHandle CylIds;
  FloatHandle Radii;

public:
  CylinderLeafWrapper(IdHandle& cylIds, FloatHandle radii)
    : CylIds(cylIds)
    , Radii(radii)
  {
  }

  template <typename Device>
  VISKORES_CONT CylinderLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return CylinderLeafIntersector<Device>(this->CylIds, this->Radii, token);
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

    Precision mag2 = viskores::Magnitude(ab);
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
{
}

CylinderIntersector::~CylinderIntersector() {}

void CylinderIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                                  viskores::cont::ArrayHandle<viskores::Id3> cylIds,
                                  viskores::cont::ArrayHandle<viskores::Float32> radii)
{
  this->Radii = radii;
  this->CylIds = cylIds;
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

  detail::CylinderLeafWrapper leafIntersector(this->CylIds, Radii);

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
