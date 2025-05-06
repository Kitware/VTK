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
#ifndef viskores_rendering_raytracing_ScalarRenderer_h
#define viskores_rendering_raytracing_ScalarRenderer_h

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

class VISKORES_RENDERING_EXPORT ScalarRenderer
{
private:
  viskores::cont::Invoker Invoke;

protected:
  std::unique_ptr<ShapeIntersector> Intersector;
  std::vector<viskores::cont::Field> Fields;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays,
                      Precision missScalar,
                      viskores::rendering::raytracing::Camera& cam);

  template <typename Precision>
  void AddBuffer(Ray<Precision>& rays, Precision missScalar, const std::string& name);

  template <typename Precision>
  void AddDepthBuffer(Ray<Precision>& rays);

public:
  VISKORES_CONT
  void SetShapeIntersector(std::unique_ptr<ShapeIntersector>&& intersector);

  VISKORES_CONT
  void AddField(const viskores::cont::Field& scalarField);

  VISKORES_CONT
  void Render(viskores::rendering::raytracing::Ray<viskores::Float32>& rays,
              viskores::Float32 missScalar,
              viskores::rendering::raytracing::Camera& cam);

  VISKORES_CONT
  void Render(viskores::rendering::raytracing::Ray<viskores::Float64>& rays,
              viskores::Float64 missScalar,
              viskores::rendering::raytracing::Camera& cam);

}; //class RayTracer
}
}
} // namespace viskores::rendering::raytracing
#endif //viskores_rendering_raytracing_ScalarRenderer_h
