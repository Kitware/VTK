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
#ifndef viskores_rendering_CanvasRayTracer_h
#define viskores_rendering_CanvasRayTracer_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/raytracing/Ray.h>

namespace viskores
{
namespace rendering
{

/// Represents the image space that is the target of rendering using the internal ray
/// tracing code.
class VISKORES_RENDERING_EXPORT CanvasRayTracer : public Canvas
{
public:
  /// Construct a canvas of a given width and height.
  CanvasRayTracer(viskores::Id width = 1024, viskores::Id height = 1024);

  ~CanvasRayTracer();

  viskores::rendering::Canvas* NewCopy() const override;

  void WriteToCanvas(const viskores::rendering::raytracing::Ray<viskores::Float32>& rays,
                     const viskores::cont::ArrayHandle<viskores::Float32>& colors,
                     const viskores::rendering::Camera& camera);

  void WriteToCanvas(const viskores::rendering::raytracing::Ray<viskores::Float64>& rays,
                     const viskores::cont::ArrayHandle<viskores::Float64>& colors,
                     const viskores::rendering::Camera& camera);
}; // class CanvasRayTracer
}
} // namespace viskores::rendering

#endif //viskores_rendering_CanvasRayTracer_h
