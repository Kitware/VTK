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
#ifndef viskores_rendering_raytracing_Glyph_Intersector_Vector_h
#define viskores_rendering_raytracing_Glyph_Intersector_Vector_h

#include <viskores/rendering/GlyphType.h>
#include <viskores/rendering/raytracing/ShapeIntersector.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

class GlyphIntersectorVector : public ShapeIntersector
{
public:
  GlyphIntersectorVector(viskores::rendering::GlyphType glyphType);
  virtual ~GlyphIntersectorVector() override;

  void SetGlyphType(viskores::rendering::GlyphType glyphType);

  void SetData(const viskores::cont::CoordinateSystem& coords,
               viskores::cont::ArrayHandle<viskores::Id> pointIds,
               viskores::cont::ArrayHandle<viskores::Vec<viskores::Float32, 3>> sizes);

  void IntersectRays(Ray<viskores::Float32>& rays, bool returnCellIndex = false) override;


  void IntersectRays(Ray<viskores::Float64>& rays, bool returnCellIndex = false) override;

  template <typename Precision>
  void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);


  template <typename Precision>
  void IntersectionDataImp(Ray<Precision>& rays,
                           const viskores::cont::Field field,
                           const viskores::Range& range);

  void IntersectionData(Ray<viskores::Float32>& rays,
                        const viskores::cont::Field field,
                        const viskores::Range& range) override;

  void IntersectionData(Ray<viskores::Float64>& rays,
                        const viskores::cont::Field field,
                        const viskores::Range& range) override;

  viskores::Id GetNumberOfShapes() const override;

  void SetArrowRadii(viskores::Float32 bodyRadius, viskores::Float32 headRadius);

protected:
  viskores::cont::ArrayHandle<viskores::Id> PointIds;
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Sizes;
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Normals;
  viskores::rendering::GlyphType GlyphType;

  viskores::Float32 ArrowBodyRadius;
  viskores::Float32 ArrowHeadRadius;
}; // class GlyphIntersectorVector

}
}
} //namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_Glyph_Intersector_Vector_h
