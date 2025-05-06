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
#ifndef viskores_rendering_raytracing_TriagnleIntersector_h
#define viskores_rendering_raytracing_TriagnleIntersector_h

#include <viskores/cont/DataSet.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/ShapeIntersector.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT TriangleIntersector : public ShapeIntersector
{
protected:
  viskores::cont::ArrayHandle<viskores::Id4> Triangles;
  bool UseWaterTight;

public:
  TriangleIntersector();

  void SetUseWaterTight(bool useIt);

  void SetData(const viskores::cont::CoordinateSystem& coords,
               viskores::cont::ArrayHandle<viskores::Id4> triangles);

  viskores::cont::ArrayHandle<viskores::Id4> GetTriangles();
  viskores::Id GetNumberOfShapes() const override;


  VISKORES_CONT void IntersectRays(Ray<viskores::Float32>& rays,
                                   bool returnCellIndex = false) override;
  VISKORES_CONT void IntersectRays(Ray<viskores::Float64>& rays,
                                   bool returnCellIndex = false) override;


  VISKORES_CONT void IntersectionData(
    Ray<viskores::Float32>& rays,
    const viskores::cont::Field scalarField,
    const viskores::Range& scalarRange = viskores::Range()) override;

  VISKORES_CONT void IntersectionData(
    Ray<viskores::Float64>& rays,
    const viskores::cont::Field scalarField,
    const viskores::Range& scalarRange = viskores::Range()) override;

  template <typename Precision>
  VISKORES_CONT void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);

  template <typename Precision>
  VISKORES_CONT void IntersectionDataImp(Ray<Precision>& rays,
                                         const viskores::cont::Field scalarField,
                                         const viskores::Range& scalarRange);

}; // class intersector
}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_TriagnleIntersector_h
