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

#include <viskores/rendering/CanvasRayTracer.h>

#include <viskores/cont/TryExecute.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace internal
{

class SurfaceConverter : public viskores::worklet::WorkletMapField
{
  viskores::Matrix<viskores::Float32, 4, 4> ViewProjMat;

public:
  VISKORES_CONT
  SurfaceConverter(const viskores::Matrix<viskores::Float32, 4, 4> viewProjMat)
    : ViewProjMat(viewProjMat)
  {
  }

  using ControlSignature =
    void(FieldIn, WholeArrayInOut, FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);
  template <typename Precision,
            typename ColorPortalType,
            typename DepthBufferPortalType,
            typename ColorBufferPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& pixelIndex,
                                ColorPortalType& colorBufferIn,
                                const Precision& inDepth,
                                const viskores::Vec<Precision, 3>& origin,
                                const viskores::Vec<Precision, 3>& dir,
                                DepthBufferPortalType& depthBuffer,
                                ColorBufferPortalType& colorBuffer,
                                const viskores::Id& index) const
  {
    viskores::Vec<Precision, 3> intersection = origin + inDepth * dir;

    viskores::Vec4f_32 point;
    point[0] = static_cast<viskores::Float32>(intersection[0]);
    point[1] = static_cast<viskores::Float32>(intersection[1]);
    point[2] = static_cast<viskores::Float32>(intersection[2]);
    point[3] = 1.f;

    viskores::Float32 depth;
    {
      viskores::Vec4f_32 newpoint;
      newpoint = viskores::MatrixMultiply(this->ViewProjMat, point);
      if (newpoint[3] > 0)
      {
        depth = 0.5f * (newpoint[2] / newpoint[3]) + 0.5f;
      }
      else
      {
        // This condition can happen when the ray is at the origin (inDepth = 0), which is a
        // singularity in the projection matrix. I'm not sure this is the right think to do since
        // it looks like depth is supposed to be between 0 and 1. It seems wrong that you would
        // ever get a ray in front of the near plane, so the "right" solution may be to fix this
        // elsewhere.
        depth = viskores::NegativeInfinity32();
      }
    }

    viskores::Vec4f_32 color;
    color[0] = static_cast<viskores::Float32>(colorBufferIn.Get(index * 4 + 0));
    color[1] = static_cast<viskores::Float32>(colorBufferIn.Get(index * 4 + 1));
    color[2] = static_cast<viskores::Float32>(colorBufferIn.Get(index * 4 + 2));
    color[3] = static_cast<viskores::Float32>(colorBufferIn.Get(index * 4 + 3));
    // blend the mapped color with existing canvas color
    viskores::Vec4f_32 inColor = colorBuffer.Get(pixelIndex);

    // if transparency exists, all alphas have been pre-multiplied
    viskores::Float32 alpha = (1.f - color[3]);
    color[0] = color[0] + inColor[0] * alpha;
    color[1] = color[1] + inColor[1] * alpha;
    color[2] = color[2] + inColor[2] * alpha;
    color[3] = inColor[3] * alpha + color[3];

    // clamp
    for (viskores::Int32 i = 0; i < 4; ++i)
    {
      color[i] = viskores::Min(1.f, viskores::Max(color[i], 0.f));
    }
    // The existing depth should already been feed into the ray mapper
    // so no color contribution will exist past the existing depth.

    depthBuffer.Set(pixelIndex, depth);
    colorBuffer.Set(pixelIndex, color);
  }
}; //class SurfaceConverter

template <typename Precision>
VISKORES_CONT void WriteToCanvas(const viskores::rendering::raytracing::Ray<Precision>& rays,
                                 const viskores::cont::ArrayHandle<Precision>& colors,
                                 const viskores::rendering::Camera& camera,
                                 viskores::rendering::CanvasRayTracer* canvas)
{
  viskores::Matrix<viskores::Float32, 4, 4> viewProjMat =
    viskores::MatrixMultiply(camera.CreateProjectionMatrix(canvas->GetWidth(), canvas->GetHeight()),
                             camera.CreateViewMatrix());

  viskores::worklet::DispatcherMapField<SurfaceConverter>(SurfaceConverter(viewProjMat))
    .Invoke(rays.PixelIdx,
            colors,
            rays.Distance,
            rays.Origin,
            rays.Dir,
            canvas->GetDepthBuffer(),
            canvas->GetColorBuffer());

  //Force the transfer so the vectors contain data from device
  canvas->GetColorBuffer().WritePortal().Get(0);
  canvas->GetDepthBuffer().WritePortal().Get(0);
}

} // namespace internal

CanvasRayTracer::CanvasRayTracer(viskores::Id width, viskores::Id height)
  : Canvas(width, height)
{
}

CanvasRayTracer::~CanvasRayTracer() {}

void CanvasRayTracer::WriteToCanvas(
  const viskores::rendering::raytracing::Ray<viskores::Float32>& rays,
  const viskores::cont::ArrayHandle<viskores::Float32>& colors,
  const viskores::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

void CanvasRayTracer::WriteToCanvas(
  const viskores::rendering::raytracing::Ray<viskores::Float64>& rays,
  const viskores::cont::ArrayHandle<viskores::Float64>& colors,
  const viskores::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

viskores::rendering::Canvas* CanvasRayTracer::NewCopy() const
{
  return new viskores::rendering::CanvasRayTracer(*this);
}
}
}
