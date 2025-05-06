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
#ifndef viskores_rendering_raytracing_Shape_Intersector_h
#define viskores_rendering_raytracing_Shape_Intersector_h

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT ShapeIntersector
{
protected:
  LinearBVH BVH;
  viskores::cont::CoordinateSystem CoordsHandle;
  viskores::Bounds ShapeBounds;
  void SetAABBs(AABBs& aabbs);

public:
  ShapeIntersector();
  virtual ~ShapeIntersector();

  //
  //  Intersect Rays finds the nearest intersection shape contained in the derived
  //  class in between min and max distances. HitIdx will be set to the local
  //  primitive id unless returnCellIndex is set to true. Cells are often
  //  decomposed into triangles and setting returnCellIndex to true will set
  //  HitIdx to the id of the cell.
  //
  virtual void IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex = false) = 0;


  virtual void IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex = false) = 0;

  //
  // Calling intersection data directly after IntersectRays popoulates
  // ray data: intersection point, surface normal, and interpolated scalar
  // value at the intersection location. Additionally, distance to intersection
  // becomes the new max distance.
  //
  virtual void IntersectionData(Ray<viskores::Float32>& rays,
                                const viskores::cont::Field scalarField,
                                const viskores::Range& scalarRange = viskores::Range()) = 0;

  virtual void IntersectionData(Ray<viskores::Float64>& rays,
                                const viskores::cont::Field scalarField,
                                const viskores::Range& scalarRange = viskores::Range()) = 0;


  template <typename Precision>
  void IntersectionPointImp(Ray<Precision>& rays);
  void IntersectionPoint(Ray<viskores::Float32>& rays);
  void IntersectionPoint(Ray<viskores::Float64>& rays);

  viskores::Bounds GetShapeBounds() const;
  virtual viskores::Id GetNumberOfShapes() const = 0;
}; // class ShapeIntersector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Shape_Intersector_h
