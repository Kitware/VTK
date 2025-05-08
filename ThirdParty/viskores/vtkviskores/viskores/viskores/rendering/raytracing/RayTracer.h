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
#ifndef viskores_rendering_raytracing_RayTracer_h
#define viskores_rendering_raytracing_RayTracer_h

#include <memory>
#include <vector>

#include <viskores/cont/DataSet.h>

#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>
namespace viskores
{
namespace rendering
{
namespace raytracing
{

class VISKORES_RENDERING_EXPORT RayTracer
{
protected:
  std::vector<std::shared_ptr<ShapeIntersector>> Intersectors;
  Camera camera;
  viskores::cont::Field ScalarField;
  viskores::Id NumberOfShapes;
  viskores::cont::ArrayHandle<viskores::Vec4f_32> ColorMap;
  viskores::Range ScalarRange;
  bool Shade;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays);

public:
  VISKORES_CONT
  RayTracer();
  VISKORES_CONT
  ~RayTracer();

  VISKORES_CONT
  Camera& GetCamera();

  VISKORES_CONT
  void AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector);

  VISKORES_CONT
  void SetField(const viskores::cont::Field& scalarField, const viskores::Range& scalarRange);

  VISKORES_CONT
  void SetColorMap(const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap);

  VISKORES_CONT
  void SetShadingOn(bool on);

  VISKORES_CONT
  void Render(viskores::rendering::raytracing::Ray<viskores::Float32>& rays);

  VISKORES_CONT
  void Render(viskores::rendering::raytracing::Ray<viskores::Float64>& rays);

  VISKORES_CONT
  viskores::Id GetNumberOfShapes() const;

  VISKORES_CONT
  void Clear();

}; //class RayTracer
}
}
} // namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_RayTracer_h
