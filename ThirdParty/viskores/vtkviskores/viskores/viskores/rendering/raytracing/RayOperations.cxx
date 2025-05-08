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
#include <viskores/rendering/raytracing/RayOperations.h>
namespace viskores
{
namespace rendering
{
namespace raytracing
{

void RayOperations::MapCanvasToRays(Ray<viskores::Float32>& rays,
                                    const viskores::rendering::Camera& camera,
                                    const viskores::rendering::CanvasRayTracer& canvas)
{
  viskores::Id width = canvas.GetWidth();
  viskores::Id height = canvas.GetHeight();
  viskores::Matrix<viskores::Float32, 4, 4> projview = viskores::MatrixMultiply(
    camera.CreateProjectionMatrix(width, height), camera.CreateViewMatrix());
  bool valid;
  viskores::Matrix<viskores::Float32, 4, 4> inverse = viskores::MatrixInverse(projview, valid);
  (void)valid; // this can be a false negative for really tiny spatial domains.
  viskores::worklet::DispatcherMapField<detail::RayMapCanvas>(
    detail::RayMapCanvas(inverse, width, height, camera.GetPosition()))
    .Invoke(rays.PixelIdx, rays.MaxDistance, rays.Origin, canvas.GetDepthBuffer());
}
}
}
} // viskores::rendering::raytacing
