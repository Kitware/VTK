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
#include <viskores/rendering/raytracing/GlyphIntersector.h>
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

class FindGlyphAABBs : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  FindGlyphAABBs() {}
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
  VISKORES_EXEC void operator()(const viskores::Id& pointId,
                                const viskores::Float32& size,
                                viskores::Float32& xmin,
                                viskores::Float32& ymin,
                                viskores::Float32& zmin,
                                viskores::Float32& xmax,
                                viskores::Float32& ymax,
                                viskores::Float32& zmax,
                                const PointPortalType& points) const
  {
    viskores::Vec3f_32 point;
    point = static_cast<viskores::Vec3f_32>(points.Get(pointId));
    viskores::Float32 absSize = viskores::Abs(size);

    xmin = point[0] - absSize;
    xmax = point[0] + absSize;
    ymin = point[1] - absSize;
    ymax = point[1] + absSize;
    zmin = point[2] - absSize;
    zmax = point[2] + absSize;
  }
}; //class FindGlyphAABBs

template <typename Device>
class GlyphLeafIntersector
{
public:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  using VecHandle = viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>>;
  using FloatPortal = typename FloatHandle::ReadPortalType;
  using VecPortal = typename VecHandle::ReadPortalType;
  IdArrayPortal PointIds;
  FloatPortal Sizes;
  viskores::rendering::GlyphType GlyphType;

  GlyphLeafIntersector() {}

  GlyphLeafIntersector(const IdHandle& pointIds,
                       const FloatHandle& sizes,
                       viskores::rendering::GlyphType glyphType,
                       viskores::cont::Token& token)
    : PointIds(pointIds.PrepareForInput(Device(), token))
    , Sizes(sizes.PrepareForInput(Device(), token))
    , GlyphType(glyphType)
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
      Precision size = Sizes.Get(idx);
      viskores::Vec<Precision, 3> point = viskores::Vec<Precision, 3>(points.Get(pointIndex));

      if (this->GlyphType == viskores::rendering::GlyphType::Sphere)
      {
        this->IntersectSphere(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
      else if (this->GlyphType == viskores::rendering::GlyphType::Cube)
      {
        this->IntersectCube(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
      else if (this->GlyphType == viskores::rendering::GlyphType::Axes)
      {
        this->IntersectAxes(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void IntersectSphere(const viskores::Vec<Precision, 3>& origin,
                                            const viskores::Vec<Precision, 3>& dir,
                                            const viskores::Vec<Precision, 3>& point,
                                            const Precision& size,
                                            const viskores::Id& pointIndex,
                                            viskores::Id& hitIndex,
                                            Precision& closestDistance,
                                            Precision& viskoresNotUsed(minU),
                                            Precision& viskoresNotUsed(minV),
                                            const Precision& minDistance) const
  {
    viskores::Vec<Precision, 3> l = point - origin;
    Precision dot1 = viskores::dot(l, dir);
    if (dot1 >= 0)
    {
      Precision d = viskores::dot(l, l) - dot1 * dot1;
      Precision r2 = size * size;
      if (d <= r2)
      {
        Precision tch = viskores::Sqrt(r2 - d);
        Precision t0 = dot1 - tch;

        if (t0 < closestDistance && t0 > minDistance)
        {
          hitIndex = pointIndex;
          closestDistance = t0;
        }
      }
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void IntersectCube(const viskores::Vec<Precision, 3>& origin,
                                          const viskores::Vec<Precision, 3>& dir,
                                          const viskores ::Vec<Precision, 3>& point,
                                          const Precision& size,
                                          const viskores::Id& pointIndex,
                                          viskores::Id& hitIndex,
                                          Precision& closestDistance,
                                          Precision& viskoresNotUsed(minU),
                                          Precision& viskoresNotUsed(minV),
                                          const Precision& minDistance) const
  {
    Precision xmin, xmax, ymin, ymax, zmin, zmax;
    this->CalculateAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);

    Precision tmin = (xmin - origin[0]) / dir[0];
    Precision tmax = (xmax - origin[0]) / dir[0];

    if (tmin > tmax)
      viskores::Swap(tmin, tmax);

    Precision tymin = (ymin - origin[1]) / dir[1];
    Precision tymax = (ymax - origin[1]) / dir[1];
    if (tymin > tymax)
      viskores::Swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
      return;

    if (tymin > tmin)
      tmin = tymin;

    if (tymax < tmax)
      tmax = tymax;

    Precision tzmin = (zmin - origin[2]) / dir[2];
    Precision tzmax = (zmax - origin[2]) / dir[2];

    if (tzmin > tzmax)
      viskores::Swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
      return;

    if (tzmin > tmin)
      tmin = tzmin;

    if (tzmax < tmax)
      tmax = tzmax;

    if (tmin < closestDistance && tmin > minDistance)
    {
      hitIndex = pointIndex;
      closestDistance = tmin;
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void IntersectAxes(const viskores::Vec<Precision, 3>& origin,
                                          const viskores::Vec<Precision, 3>& dir,
                                          const viskores::Vec<Precision, 3>& point,
                                          const Precision& size,
                                          const viskores::Id& pointIndex,
                                          viskores::Id& hitIndex,
                                          Precision& closestDistance,
                                          Precision& viskoresNotUsed(minU),
                                          Precision& viskoresNotUsed(minV),
                                          const Precision& minDistance) const
  {
    Precision xmin, xmax, ymin, ymax, zmin, zmax;
    this->CalculateAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);

    Precision t = (point[0] - origin[0]) / dir[0];
    viskores::Vec<Precision, 3> intersection = origin + t * dir;

    if ((intersection[1] >= ymin && intersection[1] <= ymax) &&
        (intersection[2] >= zmin && intersection[2] <= zmax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }

    t = (point[1] - origin[1]) / dir[1];
    intersection = origin + t * dir;
    if ((intersection[0] >= xmin && intersection[0] <= xmax) &&
        (intersection[2] >= zmin && intersection[2] <= zmax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }

    t = (point[2] - origin[2]) / dir[2];
    intersection = origin + t * dir;
    if ((intersection[0] >= xmin && intersection[0] <= xmax) &&
        (intersection[1] >= ymin && intersection[1] <= ymax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }
  }

  template <typename Precision>
  VISKORES_EXEC void CalculateAABB(const viskores::Vec<Precision, 3>& point,
                                   const Precision& size,
                                   Precision& xmin,
                                   Precision& ymin,
                                   Precision& zmin,
                                   Precision& xmax,
                                   Precision& ymax,
                                   Precision& zmax) const
  {
    Precision absSize = viskores::Abs(size);
    xmin = point[0] - absSize;
    xmax = point[0] + absSize;
    ymin = point[1] - absSize;
    ymax = point[1] + absSize;
    zmin = point[2] - absSize;
    zmax = point[2] + absSize;
  }
};

class GlyphLeafWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  using VecHandle = viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>>;
  IdHandle PointIds;
  FloatHandle Sizes;
  viskores::rendering::GlyphType GlyphType;

public:
  GlyphLeafWrapper(IdHandle& pointIds, FloatHandle sizes, viskores::rendering::GlyphType glyphType)
    : PointIds(pointIds)
    , Sizes(sizes)
    , GlyphType(glyphType)
  {
  }

  template <typename Device>
  VISKORES_CONT GlyphLeafIntersector<Device> PrepareForExecution(Device,
                                                                 viskores::cont::Token& token) const
  {
    return GlyphLeafIntersector<Device>(this->PointIds, this->Sizes, this->GlyphType, token);
  }
}; // class GlyphLeafWrapper

class CalculateGlyphNormals : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CalculateGlyphNormals(viskores::rendering::GlyphType glyphType)
    : GlyphType(glyphType)
  {
  }

  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename Precision,
            typename PointPortalType,
            typename IndicesPortalType,
            typename SizesPortalType>
  VISKORES_EXEC inline void operator()(const viskores::Id& hitIndex,
                                       const viskores::Vec<Precision, 3>& rayDir,
                                       const viskores::Vec<Precision, 3>& intersection,
                                       Precision& normalX,
                                       Precision& normalY,
                                       Precision& normalZ,
                                       const PointPortalType& points,
                                       const IndicesPortalType& indicesPortal,
                                       const SizesPortalType& sizesPortal) const
  {
    if (hitIndex < 0)
      return;

    viskores::Id pointId = indicesPortal.Get(hitIndex);
    viskores::Vec<Precision, 3> point = points.Get(pointId);
    Precision size = sizesPortal.Get(hitIndex);

    if (this->GlyphType == viskores::rendering::GlyphType::Sphere)
    {
      this->CalculateNormalForSphere(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
    else if (this->GlyphType == viskores::rendering::GlyphType::Cube)
    {
      this->CalculateNormalForCube(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
    else if (this->GlyphType == viskores::rendering::GlyphType::Axes)
    {
      this->CalculateNormalForAxes(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
  }

  template <typename Precision>
  VISKORES_EXEC inline void CalculateNormalForSphere(
    const viskores::Vec<Precision, 3>& rayDir,
    const viskores::Vec<Precision, 3>& intersection,
    const viskores::Vec<Precision, 3>& point,
    const Precision& viskoresNotUsed(size),
    Precision& normalX,
    Precision& normalY,
    Precision& normalZ) const
  {
    viskores::Vec<Precision, 3> normal = intersection - point;
    viskores::Normalize(normal);

    // Flip normal if it is pointing the wrong way
    if (viskores::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VISKORES_EXEC inline void CalculateNormalForCube(const viskores::Vec<Precision, 3>& rayDir,
                                                   const viskores::Vec<Precision, 3>& intersection,
                                                   const viskores::Vec<Precision, 3>& point,
                                                   const Precision& size,
                                                   Precision& normalX,
                                                   Precision& normalY,
                                                   Precision& normalZ) const
  {
    viskores::Vec<Precision, 3> lp = intersection - point;

    // Localize the intersection point to the surface of the cube.
    // One of the components will be 1 or -1 based on the face it lies on
    lp = lp * (1.0f / size);

    Precision eps = 1e-4f;
    viskores::Vec<Precision, 3> normal{ 0.0f, 0.0f, 0.0f };
    normal[0] = (viskores::Abs(viskores::Abs(lp[0]) - 1.0f) <= eps) ? lp[0] : 0.0f;
    normal[1] = (viskores::Abs(viskores::Abs(lp[1]) - 1.0f) <= eps) ? lp[1] : 0.0f;
    normal[2] = (viskores::Abs(viskores::Abs(lp[2]) - 1.0f) <= eps) ? lp[2] : 0.0f;
    Precision magSquared = viskores::MagnitudeSquared(normal);
    if (magSquared > eps)
    {
      normal = viskores::RSqrt(magSquared) * normal;
    }

    // Flip normal if it is pointing the wrong way
    if (viskores::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VISKORES_EXEC inline void CalculateNormalForAxes(const viskores::Vec<Precision, 3>& rayDir,
                                                   const viskores::Vec<Precision, 3>& intersection,
                                                   const viskores::Vec<Precision, 3>& point,
                                                   const Precision& viskoresNotUsed(size),
                                                   Precision& normalX,
                                                   Precision& normalY,
                                                   Precision& normalZ) const
  {
    viskores::Vec<Precision, 3> normal{ 0.0f, 0.0f, 0.0f };

    if (this->ApproxEquals(point[0], intersection[0]))
    {
      normal[0] = 1.0f;
    }
    else if (this->ApproxEquals(point[1], intersection[1]))
    {
      normal[1] = 1.0f;
    }
    else
    {
      normal[2] = 1.0f;
    }

    // Flip normal if it is pointing the wrong way
    if (viskores::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VISKORES_EXEC inline Precision ApproxEquals(Precision x, Precision y, Precision eps = 1e-5f) const
  {
    return viskores::Abs(x - y) <= eps;
  }

  viskores::rendering::GlyphType GlyphType;
}; //class CalculateGlyphNormals

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
  template <typename ScalarPortalType, typename IndicesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                Precision& scalar,
                                const ScalarPortalType& scalars,
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
}; //class GetScalars
}

GlyphIntersector::GlyphIntersector(viskores::rendering::GlyphType glyphType)
  : ShapeIntersector()
{
  this->SetGlyphType(glyphType);
}

GlyphIntersector::~GlyphIntersector() {}

void GlyphIntersector::SetGlyphType(viskores::rendering::GlyphType glyphType)
{
  this->GlyphType = glyphType;
}

void GlyphIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                               viskores::cont::ArrayHandle<viskores::Id> pointIds,
                               viskores::cont::ArrayHandle<viskores::Float32> sizes)
{
  this->PointIds = pointIds;
  this->Sizes = sizes;
  this->CoordsHandle = coords;
  AABBs AABB;
  viskores::worklet::DispatcherMapField<detail::FindGlyphAABBs>(detail::FindGlyphAABBs())
    .Invoke(PointIds,
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

void GlyphIntersector::IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void GlyphIntersector::IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void GlyphIntersector::IntersectRaysImp(Ray<Precision>& rays, bool viskoresNotUsed(returnCellIndex))
{
  detail::GlyphLeafWrapper leafIntersector(this->PointIds, Sizes, this->GlyphType);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void GlyphIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                           const viskores::cont::Field scalarField,
                                           const viskores::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = scalarField.IsCellField() || scalarField.IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue(
      "GlyphIntersector: Field not accociated with a cell set or field");
  }

  viskores::worklet::DispatcherMapField<detail::CalculateGlyphNormals>(
    detail::CalculateGlyphNormals(this->GlyphType))
    .Invoke(rays.HitIdx,
            rays.Dir,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds,
            Sizes);

  viskores::worklet::DispatcherMapField<detail::GetScalars<Precision>>(
    detail::GetScalars<Precision>(viskores::Float32(scalarRange.Min),
                                  viskores::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            viskores::rendering::raytracing::GetScalarFieldArray(scalarField),
            PointIds);
}

void GlyphIntersector::IntersectionData(Ray<viskores::Float32>& rays,
                                        const viskores::cont::Field scalarField,
                                        const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void GlyphIntersector::IntersectionData(Ray<viskores::Float64>& rays,
                                        const viskores::cont::Field scalarField,
                                        const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

viskores::Id GlyphIntersector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
