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
#ifndef viskores_rendering_raytracing_Quad_Intersector_h
#define viskores_rendering_raytracing_Quad_Intersector_h

#include <viskores/cont/Algorithm.h>
#include <viskores/rendering/raytracing/ShapeIntersector.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
namespace detail
{
}
class QuadIntersector : public ShapeIntersector
{
protected:
  viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> QuadIds;

public:
  QuadIntersector();
  virtual ~QuadIntersector() override;


  void SetData(const viskores::cont::CoordinateSystem& coords,
               viskores::cont::ArrayHandle<viskores::Vec<viskores::Id, 5>> quadIds);

  void IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex = false) override;


  void IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex = false) override;

  template <typename Precision>
  void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);


  template <typename Precision>
  void IntersectionDataImp(Ray<Precision>& rays,
                           const viskores::cont::Field scalarField,
                           const viskores::Range& scalarRange);

  void IntersectionData(Ray<viskores::Float32>& rays,
                        const viskores::cont::Field scalarField,
                        const viskores::Range& scalarRange = viskores::Range()) override;

  void IntersectionData(Ray<viskores::Float64>& rays,
                        const viskores::cont::Field scalarField,
                        const viskores::Range& scalarRange = viskores::Range()) override;

  viskores::Id GetNumberOfShapes() const override;
}; // class ShapeIntersector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Shape_Intersector_h
