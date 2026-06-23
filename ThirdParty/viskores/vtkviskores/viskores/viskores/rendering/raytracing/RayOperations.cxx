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

void RayOperations::MapCanvasToRays(
  Ray<viskores::Float32>& rays,
  const viskores::rendering::raytracing::Camera& camera,
  const viskores::cont::ArrayHandle<viskores::Float32>& depthBuffer)
{
  viskores::Id width = camera.GetWidth();
  viskores::Id height = camera.GetHeight();
  VISKORES_ASSERT(width * height == depthBuffer.GetNumberOfValues());
  viskores::Matrix<viskores::Float32, 4, 4> projview = camera.GetViewProjectionMatrix();
  bool valid;
  viskores::Matrix<viskores::Float32, 4, 4> inverse = viskores::MatrixInverse(projview, valid);
  (void)valid; // this can be a false negative for really tiny spatial domains.
  viskores::worklet::DispatcherMapField<detail::RayMapCanvas>(
    detail::RayMapCanvas(inverse, width, height, camera.GetPosition()))
    .Invoke(rays.PixelIdx, rays.MaxDistance, rays.Origin, depthBuffer);
}
}
}
} // viskores::rendering::raytacing
