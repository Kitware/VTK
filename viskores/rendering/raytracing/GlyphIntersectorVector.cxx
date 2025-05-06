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
#include <viskores/rendering/raytracing/GlyphIntersectorVector.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>

// This line is at the end to prevent warnings when building for CUDA
#include <viskores/Swap.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{
static constexpr viskores::Float32 ARROW_BODY_SIZE = 0.75f;

class FindGlyphVectorAABBs : public viskores::worklet::WorkletMapField
{
  viskores::rendering::GlyphType GlyphType;
  viskores::Float32 ArrowBodyRadius;
  viskores::Float32 ArrowHeadRadius;

public:
  using ControlSignature = void(FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  VISKORES_CONT
  FindGlyphVectorAABBs(viskores::rendering::GlyphType glyphType,
                       viskores::Float32 bodyRadius,
                       viskores::Float32 headRadius)
    : GlyphType(glyphType)
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
  {
  }

  template <typename PointPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                const viskores::Vec3f_32& size,
                                viskores::Float32& xmin,
                                viskores::Float32& ymin,
                                viskores::Float32& zmin,
                                viskores::Float32& xmax,
                                viskores::Float32& ymax,
                                viskores::Float32& zmax,
                                const PointPortalType& points) const
  {
    viskores::Vec3f_32 point = static_cast<viskores::Vec3f_32>(points.Get(pointId));
    xmin = point[0];
    xmax = point[0];
    ymin = point[1];
    ymax = point[1];
    zmin = point[2];
    zmax = point[2];

    if (this->GlyphType == viskores::rendering::GlyphType::Arrow)
    {
      this->CalculateArrowAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);
    }
  }

  VISKORES_EXEC inline void CalculateArrowAABB(const viskores::Vec3f_32& point,
                                               const viskores::Vec3f_32& size,
                                               viskores::Float32& xmin,
                                               viskores::Float32& ymin,
                                               viskores::Float32& zmin,
                                               viskores::Float32& xmax,
                                               viskores::Float32& ymax,
                                               viskores::Float32& zmax) const
  {
    viskores::Vec3f_32 body_pa = point;
    viskores::Vec3f_32 body_pb = body_pa + ARROW_BODY_SIZE * size;
    viskores::Vec3f_32 head_pa = body_pb;
    viskores::Vec3f_32 head_pb = point + size;

    this->CylinderAABB(body_pa, body_pb, this->ArrowBodyRadius, xmin, ymin, zmin, xmax, ymax, zmax);
    this->ConeAABB(
      head_pa, head_pb, this->ArrowHeadRadius, 0.0f, xmin, ymin, zmin, xmax, ymax, zmax);
  }

  VISKORES_EXEC inline void CylinderAABB(const viskores::Vec3f_32& pa,
                                         const viskores::Vec3f_32& pb,
                                         const viskores::Float32& ra,
                                         viskores::Float32& xmin,
                                         viskores::Float32& ymin,
                                         viskores::Float32& zmin,
                                         viskores::Float32& xmax,
                                         viskores::Float32& ymax,
                                         viskores::Float32& zmax) const
  {
    viskores::Vec3f_32 a = pb - pa;
    viskores::Vec3f_32 e_prime = a * a / viskores::Dot(a, a);
    viskores::Vec3f_32 e = ra * viskores::Sqrt(1.0f - e_prime);

    viskores::Vec3f_32 pa1 = pa - e;
    viskores::Vec3f_32 pa2 = pa + e;
    viskores::Vec3f_32 pb1 = pb - e;
    viskores::Vec3f_32 pb2 = pb + e;

    xmin = viskores::Min(xmin, viskores::Min(pa1[0], pb1[0]));
    ymin = viskores::Min(ymin, viskores::Min(pa1[1], pb1[1]));
    zmin = viskores::Min(zmin, viskores::Min(pa1[2], pb1[2]));
    xmax = viskores::Max(xmax, viskores::Max(pa2[0], pb2[0]));
    ymax = viskores::Max(ymax, viskores::Max(pa2[1], pb2[1]));
    zmax = viskores::Max(zmax, viskores::Max(pa2[2], pb2[2]));
  }

  VISKORES_EXEC inline void ConeAABB(const viskores::Vec3f_32& pa,
                                     const viskores::Vec3f_32& pb,
                                     const viskores::Float32& ra,
                                     const viskores::Float32& rb,
                                     viskores::Float32& xmin,
                                     viskores::Float32& ymin,
                                     viskores::Float32& zmin,
                                     viskores::Float32& xmax,
                                     viskores::Float32& ymax,
                                     viskores::Float32& zmax) const
  {
    viskores::Vec3f_32 a = pb - pa;
    viskores::Vec3f_32 e_prime = a * a / viskores::Dot(a, a);
    viskores::Vec3f_32 e = viskores::Sqrt(1.0f - e_prime);

    viskores::Vec3f_32 pa1 = pa - e * ra;
    viskores::Vec3f_32 pa2 = pa + e * ra;
    viskores::Vec3f_32 pb1 = pb - e * rb;
    viskores::Vec3f_32 pb2 = pb + e * rb;

    xmin = viskores::Min(xmin, viskores::Min(pa1[0], pb1[0]));
    ymin = viskores::Min(ymin, viskores::Min(pa1[1], pb1[1]));
    zmin = viskores::Min(zmin, viskores::Min(pa1[2], pb1[2]));
    xmax = viskores::Max(xmax, viskores::Max(pa2[0], pb2[0]));
    ymax = viskores::Max(ymax, viskores::Max(pa2[1], pb2[1]));
    zmax = viskores::Max(zmax, viskores::Max(pa2[2], pb2[2]));
  }
}; //class FindGlyphVectorAABBs

template <typename Device>
class GlyphVectorLeafIntersector
{
public:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using Vec3f_32Handle = viskores::cont::ArrayHandle<viskores::Vec3f_32>;
  using Vec3f_32Portal = typename Vec3f_32Handle::ReadPortalType;

  viskores::rendering::GlyphType GlyphType;
  IdArrayPortal PointIds;
  Vec3f_32Portal Sizes;
  viskores::Float32 ArrowBodyRadius;
  viskores::Float32 ArrowHeadRadius;

  GlyphVectorLeafIntersector() = default;

  GlyphVectorLeafIntersector(viskores::rendering::GlyphType glyphType,
                             const IdHandle& pointIds,
                             const Vec3f_32Handle& sizes,
                             viskores::Float32 bodyRadius,
                             viskores::Float32 headRadius,
                             viskores::cont::Token& token)
    : GlyphType(glyphType)
    , PointIds(pointIds.PrepareForInput(Device(), token))
    , Sizes(sizes.PrepareForInput(Device(), token))
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VISKORES_EXEC inline void IntersectLeaf(const viskores::Int32& currentNode,
                                          const viskores::Vec<Precision, 3>& origin,
                                          const viskores::Vec<Precision, 3>& dir,
                                          const PointPortalType& points,
                                          viskores::Id& hitIndex,
                                          Precision& closestDistance,
                                          Precision& minU,
                                          Precision& minV,
                                          LeafPortalType leafs,
                                          const Precision& minDistance) const
  {
    const viskores::Id glyphCount = leafs.Get(currentNode);

    for (viskores::Id i = 1; i <= glyphCount; ++i)
    {
      const viskores::Id idx = leafs.Get(currentNode + i);
      viskores::Id pointIndex = PointIds.Get(idx);
      viskores::Vec<Precision, 3> size = Sizes.Get(idx);
      viskores::Vec<Precision, 3> point = viskores::Vec<Precision, 3>(points.Get(pointIndex));

      if (this->GlyphType == viskores::rendering::GlyphType::Arrow)
      {
        this->IntersectArrow(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void IntersectArrow(const viskores::Vec<Precision, 3>& origin,
                                           const viskores::Vec<Precision, 3>& dir,
                                           const viskores::Vec<Precision, 3>& point,
                                           const viskores::Vec<Precision, 3>& size,
                                           const viskores::Id& pointIndex,
                                           viskores::Id& hitIndex,
                                           Precision& closestDistance,
                                           Precision& minU,
                                           Precision& minV,
                                           const Precision& minDistance) const
  {
    using Vec2 = viskores::Vec<Precision, 2>;
    using Vec3 = viskores::Vec<Precision, 3>;
    using Vec4 = viskores::Vec<Precision, 4>;

    Vec3 body_pa = point;
    Vec3 body_pb = body_pa + ARROW_BODY_SIZE * size;
    Vec3 head_pa = body_pb;
    Vec3 head_pb = point + size;

    Vec4 bodyIntersection =
      this->IntersectCylinder(origin, dir, body_pa, body_pb, Precision(this->ArrowBodyRadius));
    Vec4 headIntersection = this->IntersectCone(
      origin, dir, head_pa, head_pb, Precision(this->ArrowHeadRadius), Precision(0.0f));

    bool bodyHit = bodyIntersection[0] >= minDistance;
    bool headHit = headIntersection[0] >= minDistance;
    if (bodyHit && !headHit)
    {
      Precision t = bodyIntersection[0];
      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = bodyIntersection[1];
        minV = bodyIntersection[2];
      }
    }
    else if (!bodyHit && headHit)
    {
      Precision t = headIntersection[0];
      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = headIntersection[1];
        minV = headIntersection[2];
      }
    }
    else if (bodyHit || headHit)
    {
      Precision t1 = bodyIntersection[0];
      Precision t2 = headIntersection[0];

      Precision t = t1;
      Vec2 partialNormal = { bodyIntersection[1], bodyIntersection[2] };
      if (t2 < t)
      {
        t = t2;
        partialNormal[0] = headIntersection[1];
        partialNormal[1] = headIntersection[2];
      }

      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = partialNormal[0];
        minV = partialNormal[1];
      }
    }
  }

  template <typename Precision>
  VISKORES_EXEC viskores::Vec4f_32 IntersectCylinder(const viskores::Vec<Precision, 3>& ro,
                                                     const viskores::Vec<Precision, 3>& rd,
                                                     const viskores::Vec<Precision, 3>& pa,
                                                     const viskores::Vec<Precision, 3>& pb,
                                                     const Precision& ra) const
  {
    using Vec3 = viskores::Vec<Precision, 3>;
    using Vec4 = viskores::Vec<Precision, 4>;

    const Vec4 NO_HIT{ -1.0f, -1.0f, -1.0f, -1.0f };

    Vec3 cc = 0.5f * (pa + pb);
    Precision ch = viskores::Magnitude(pb - pa);
    Vec3 ca = (pb - pa) / ch;
    ch *= 0.5f;

    Vec3 oc = ro - cc;

    Precision card = viskores::Dot(ca, rd);
    Precision caoc = viskores::Dot(ca, oc);

    Precision a = 1.0f - card * card;
    Precision b = viskores::Dot(oc, rd) - caoc * card;
    Precision c = viskores::Dot(oc, oc) - caoc * caoc - ra * ra;
    Precision h = b * b - a * c;
    if (h < 0.0f)
    {
      return NO_HIT;
    }

    h = viskores::Sqrt(h);
    Precision t1 = (-b - h) / a;
    /* Precision t2 = (-b + h) / a; // exit point */

    Precision y = caoc + t1 * card;

    // body
    if (viskores::Abs(y) < ch)
    {
      viskores::Vec3f_32 normal = viskores::Normal(oc + t1 * rd - ca * y);
      return viskores::Vec4f_32(
        static_cast<viskores::Float32>(t1), normal[0], normal[1], normal[2]);
    }

    // bottom cap
    Precision sy = -1;
    Precision tp = (sy * ch - caoc) / card;
    if (viskores::Abs(b + a * tp) < h)
    {
      viskores::Vec3f_32 normal = viskores::Normal(ca * sy);
      return viskores::Vec4f_32(
        static_cast<viskores::Float32>(tp), normal[0], normal[1], normal[2]);
    }

    // top cap
    sy = 1;
    tp = (sy * ch - caoc) / card;
    if (viskores::Abs(b + a * tp) < h)
    {
      viskores::Vec3f_32 normal = viskores::Normal(ca * sy);
      return viskores::Vec4f_32(
        static_cast<viskores::Float32>(tp), normal[0], normal[1], normal[2]);
    }

    return NO_HIT;
  }

  template <typename Precision>
  VISKORES_EXEC viskores::Vec4f_32 IntersectCone(const viskores::Vec<Precision, 3>& ro,
                                                 const viskores::Vec<Precision, 3>& rd,
                                                 const viskores::Vec<Precision, 3>& pa,
                                                 const viskores::Vec<Precision, 3>& pb,
                                                 const Precision& ra,
                                                 const Precision& rb) const
  {
    using Vec3 = viskores::Vec<Precision, 3>;
    using Vec4 = viskores::Vec<Precision, 4>;

    const Vec4 NO_HIT{ -1.0f, -1.0f, -1.0f, -1.0f };

    Vec3 ba = pb - pa;
    Vec3 oa = ro - pa;
    Vec3 ob = ro - pb;

    Precision m0 = viskores::Dot(ba, ba);
    Precision m1 = viskores::Dot(oa, ba);
    Precision m2 = viskores::Dot(ob, ba);
    Precision m3 = viskores::Dot(rd, ba);

    //caps
    if (m1 < 0.0)
    {
      Vec3 m11 = oa * m3 - rd * m1;
      Precision m12 = ra * ra * m3 * m3;
      if (viskores::Dot(m11, m11) < m12)
      {
        Precision t = -m1 / m3;
        Vec3 normal = -ba * 1.0f / viskores::Sqrt(m0);
        return Vec4(t, normal[0], normal[1], normal[2]);
      }
    }
    else if (m2 > 0.0)
    {
      Vec3 m21 = ob * m3 - rd * m2;
      Precision m22 = rb * rb * m3 * m3;
      if (viskores::Dot(m21, m21) < m22)
      {
        Precision t = -m2 / m3;
        Vec3 normal = ba * 1.0f / viskores::Sqrt(m0);
        return Vec4(t, normal[0], normal[1], normal[2]);
      }
    }

    // body
    Precision rr = ra - rb;
    Precision hy = m0 + rr * rr;
    Precision m4 = viskores::Dot(rd, oa);
    Precision m5 = viskores::Dot(oa, oa);

    Precision k2 = m0 * m0 - m3 * m3 * hy;
    Precision k1 = m0 * m0 * m4 - m1 * m3 * hy + m0 * ra * (rr * m3 * 1.0f);
    Precision k0 = m0 * m0 * m5 - m1 * m1 * hy + m0 * ra * (rr * m1 * 2.0f - m0 * ra);

    Precision h = k1 * k1 - k2 * k0;
    if (h < 0.0)
    {
      return NO_HIT;
    }

    Precision t = (-k1 - sqrt(h)) / k2;
    Precision y = m1 + t * m3;

    if (y > 0.0 && y < m0)
    {
      Vec3 normal = viskores::Normal(m0 * (m0 * (oa + t * rd) + rr * ba * ra) - ba * hy * y);
      return Vec4(t, normal[0], normal[1], normal[2]);
    }

    return NO_HIT;
  }
};

class GlyphVectorLeafWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using Vec3f_32Handle = viskores::cont::ArrayHandle<viskores::Vec3f_32>;
  viskores::rendering::GlyphType GlyphType;
  IdHandle PointIds;
  Vec3f_32Handle Sizes;
  viskores::Float32 ArrowBodyRadius;
  viskores::Float32 ArrowHeadRadius;

public:
  GlyphVectorLeafWrapper(viskores::rendering::GlyphType glyphType,
                         IdHandle& pointIds,
                         Vec3f_32Handle& sizes,
                         viskores::Float32 bodyRadius,
                         viskores::Float32 headRadius)
    : GlyphType(glyphType)
    , PointIds(pointIds)
    , Sizes(sizes)
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
  {
  }

  template <typename Device>
  VISKORES_CONT GlyphVectorLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return GlyphVectorLeafIntersector<Device>(this->GlyphType,
                                              this->PointIds,
                                              this->Sizes,
                                              this->ArrowBodyRadius,
                                              this->ArrowHeadRadius,
                                              token);
  }
};

class CalculateGlyphVectorNormals : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CalculateGlyphVectorNormals(viskores::rendering::GlyphType glyphType)
    : GlyphType(glyphType)
  {
  }

  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11);

  template <typename Precision,
            typename PointPortalType,
            typename IndicesPortalType,
            typename SizesPortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id& hitIndex,
                                       const viskores::Vec<Precision, 3>& rayDir,
                                       const viskores::Vec<Precision, 3>& intersection,
                                       const Precision& u,
                                       const Precision& v,
                                       Precision& normalX,
                                       Precision& normalY,
                                       Precision& normalZ,
                                       const PointPortalType& viskoresNotUsed(points),
                                       const IndicesPortalType& viskoresNotUsed(indicesPortal),
                                       const SizesPortalType& viskoresNotUsed(sizesPortal)) const
  {
    if (hitIndex < 0)
      return;

    if (this->GlyphType == viskores::rendering::GlyphType::Arrow)
    {
      this->CalculateArrowNormal(rayDir, intersection, u, v, normalX, normalY, normalZ);
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void CalculateArrowNormal(
    const viskores::Vec<Precision, 3>& rayDir,
    const viskores::Vec<Precision, 3>& viskoresNotUsed(intersection),
    const Precision& u,
    const Precision& v,
    Precision& normalX,
    Precision& normalY,
    Precision& normalZ) const
  {
    viskores::Vec<Precision, 3> normal;
    normal[0] = u;
    normal[1] = v;
    normal[2] = 1.0f - (normalX * normalX) - (normalY * normalY);

    if (viskores::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  viskores::rendering::GlyphType GlyphType;
}; //class CalculateGlyphVectorNormals

template <typename Precision>
class GetScalars : public viskores::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Normalize;

public:
  VISKORES_CONT
  GetScalars(const viskores::Float32& minScalar, const viskores::Float32& maxScalar)
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
  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename FieldPortalType, typename IndicesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                Precision& scalar,
                                const FieldPortalType& scalars,
                                const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Id pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId));
    if (Normalize)
    {
      scalar = (scalar - this->MinScalar) * this->InvDeltaScalar;
    }
  }
}; //class GetScalar

} // namespace

GlyphIntersectorVector::GlyphIntersectorVector(viskores::rendering::GlyphType glyphType)
  : ShapeIntersector()
  , ArrowBodyRadius(0.004f)
  , ArrowHeadRadius(0.008f)
{
  this->SetGlyphType(glyphType);
}

GlyphIntersectorVector::~GlyphIntersectorVector() {}

void GlyphIntersectorVector::SetGlyphType(viskores::rendering::GlyphType glyphType)
{
  this->GlyphType = glyphType;
}

void GlyphIntersectorVector::SetData(const viskores::cont::CoordinateSystem& coords,
                                     viskores::cont::ArrayHandle<viskores::Id> pointIds,
                                     viskores::cont::ArrayHandle<viskores::Vec3f_32> sizes)
{
  this->PointIds = pointIds;
  this->Sizes = sizes;
  this->CoordsHandle = coords;
  AABBs AABB;
  viskores::cont::Invoker invoker;
  invoker(
    detail::FindGlyphVectorAABBs{ this->GlyphType, this->ArrowBodyRadius, this->ArrowHeadRadius },
    PointIds,
    Sizes,
    AABB.xmins,
    AABB.ymins,
    AABB.zmins,
    AABB.xmaxs,
    AABB.ymaxs,
    AABB.zmaxs,
    CoordsHandle);

  this->SetAABBs(AABB);
}

void GlyphIntersectorVector::IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void GlyphIntersectorVector::IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void GlyphIntersectorVector::IntersectRaysImp(Ray<Precision>& rays,
                                              bool viskoresNotUsed(returnCellIndex))
{
  detail::GlyphVectorLeafWrapper leafIntersector(
    this->GlyphType, this->PointIds, this->Sizes, this->ArrowBodyRadius, this->ArrowHeadRadius);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void GlyphIntersectorVector::IntersectionDataImp(Ray<Precision>& rays,
                                                 const viskores::cont::Field field,
                                                 const viskores::Range& range)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = field.IsCellField() || field.IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue(
      "GlyphIntersectorVector: Field not accociated with a cell set or field");
  }

  viskores::worklet::DispatcherMapField<detail::CalculateGlyphVectorNormals>(
    detail::CalculateGlyphVectorNormals(this->GlyphType))
    .Invoke(rays.HitIdx,
            rays.Dir,
            rays.Intersection,
            rays.U,
            rays.V,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds,
            Sizes);

  viskores::worklet::DispatcherMapField<detail::GetScalars<Precision>>(
    detail::GetScalars<Precision>(viskores::Float32(range.Min), viskores::Float32(range.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            viskores::rendering::raytracing::GetScalarFieldArray(field),
            PointIds);
}

void GlyphIntersectorVector::IntersectionData(Ray<viskores::Float32>& rays,
                                              const viskores::cont::Field field,
                                              const viskores::Range& range)
{
  IntersectionDataImp(rays, field, range);
}

void GlyphIntersectorVector::IntersectionData(Ray<viskores::Float64>& rays,
                                              const viskores::cont::Field field,
                                              const viskores::Range& range)
{
  IntersectionDataImp(rays, field, range);
}

viskores::Id GlyphIntersectorVector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}

void GlyphIntersectorVector::SetArrowRadii(viskores::Float32 bodyRadius,
                                           viskores::Float32 headRadius)
{
  this->ArrowHeadRadius = headRadius;
  this->ArrowBodyRadius = bodyRadius;
}

}
}
} //namespace viskores::rendering::raytracing
