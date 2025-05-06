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
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/DispatcherMapTopology.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class FindSphereAABBs : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  FindSphereAABBs() {}
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
  VISKORES_EXEC void operator()(const viskores::Id pointId,
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
    viskores::Vec3f_32 point;
    viskores::Vec3f_32 temp;
    point = static_cast<viskores::Vec3f_32>(points.Get(pointId));

    temp[0] = radius;
    temp[1] = 0.f;
    temp[2] = 0.f;

    viskores::Vec3f_32 p = point + temp;
    //set first point to max and min
    xmin = p[0];
    xmax = p[0];
    ymin = p[1];
    ymax = p[1];
    zmin = p[2];
    zmax = p[2];

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
class SphereLeafIntersector
{
public:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  using FloatPortal = typename FloatHandle::ReadPortalType;
  IdArrayPortal PointIds;
  FloatPortal Radii;

  SphereLeafIntersector() {}

  SphereLeafIntersector(const IdHandle& pointIds,
                        const FloatHandle& radii,
                        viskores::cont::Token& token)
    : PointIds(pointIds.PrepareForInput(Device(), token))
    , Radii(radii.PrepareForInput(Device(), token))
  {
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
    const viskores::Id sphereCount = leafs.Get(currentNode);
    for (viskores::Id i = 1; i <= sphereCount; ++i)
    {
      const viskores::Id sphereIndex = leafs.Get(currentNode + i);
      viskores::Id pointIndex = PointIds.Get(sphereIndex);
      viskores::Float32 radius = Radii.Get(sphereIndex);
      viskores::Vec<Precision, 3> center = viskores::Vec<Precision, 3>(points.Get(pointIndex));

      viskores::Vec<Precision, 3> l = center - origin;

      Precision dot1 = viskores::dot(l, dir);

      if (dot1 >= 0)
      {
        Precision d = viskores::dot(l, l) - dot1 * dot1;
        Precision r2 = radius * radius;
        if (d <= r2)
        {
          Precision tch = viskores::Sqrt(r2 - d);
          Precision t0 = dot1 - tch;
          //float t1 = dot1+tch; /* if t1 is > 0 and t0<0 then the ray is inside the sphere.

          if (t0 < closestDistance && t0 > minDistance)
          {
            hitIndex = pointIndex;
            closestDistance = t0;
          }
        }
      }
    } // for
  }
};

class SphereLeafWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using IdHandle = viskores::cont::ArrayHandle<viskores::Id>;
  using FloatHandle = viskores::cont::ArrayHandle<viskores::Float32>;
  IdHandle PointIds;
  FloatHandle Radii;

public:
  SphereLeafWrapper(IdHandle& pointIds, FloatHandle radii)
    : PointIds(pointIds)
    , Radii(radii)
  {
  }

  template <typename Device>
  VISKORES_CONT SphereLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return SphereLeafIntersector<Device>(this->PointIds, this->Radii, token);
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

    viskores::Id pointId = indicesPortal.Get(hitIndex);
    viskores::Vec<Precision, 3> center = points.Get(pointId);

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
}; //class GetScalar

} // namespace detail

SphereIntersector::SphereIntersector()
  : ShapeIntersector()
{
}

SphereIntersector::~SphereIntersector() {}

void SphereIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                                viskores::cont::ArrayHandle<viskores::Id> pointIds,
                                viskores::cont::ArrayHandle<viskores::Float32> radii)
{
  this->PointIds = pointIds;
  this->Radii = radii;
  this->CoordsHandle = coords;
  AABBs AABB;
  viskores::worklet::DispatcherMapField<detail::FindSphereAABBs>(detail::FindSphereAABBs())
    .Invoke(PointIds,
            Radii,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

void SphereIntersector::IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void SphereIntersector::IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void SphereIntersector::IntersectRaysImp(Ray<Precision>& rays,
                                         bool viskoresNotUsed(returnCellIndex))
{

  detail::SphereLeafWrapper leafIntersector(this->PointIds, Radii);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void SphereIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                            const viskores::cont::Field scalarField,
                                            const viskores::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = scalarField.IsCellField() || scalarField.IsPointField();
  if (!isSupportedField)
  {
    throw viskores::cont::ErrorBadValue(
      "SphereIntersector: Field not accociated with a cell set or field");
  }

  viskores::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds);

  viskores::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(viskores::Float32(scalarRange.Min),
                                 viskores::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            viskores::rendering::raytracing::GetScalarFieldArray(scalarField),
            PointIds);
}

void SphereIntersector::IntersectionData(Ray<viskores::Float32>& rays,
                                         const viskores::cont::Field scalarField,
                                         const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void SphereIntersector::IntersectionData(Ray<viskores::Float64>& rays,
                                         const viskores::cont::Field scalarField,
                                         const viskores::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

viskores::Id SphereIntersector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
