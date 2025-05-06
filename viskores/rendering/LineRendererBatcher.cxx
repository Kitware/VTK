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

#include <viskores/rendering/LineRendererBatcher.h>

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace
{
using ColorsArrayHandle = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
using PointsArrayHandle = viskores::cont::ArrayHandle<viskores::Vec3f_32>;

struct RenderLine : public viskores::worklet::WorkletMapField
{
  using ColorBufferType = viskores::rendering::Canvas::ColorBufferType;
  using DepthBufferType = viskores::rendering::Canvas::DepthBufferType;

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  VISKORES_CONT
  RenderLine() {}

  VISKORES_CONT
  RenderLine(viskores::Id width, viskores::Id height)
    : Width(width)
    , Height(height)
  {
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VISKORES_EXEC void operator()(const viskores::Vec3f_32& start,
                                const viskores::Vec3f_32& end,
                                const viskores::Vec4f_32& color,
                                ColorBufferPortal& colorBuffer,
                                DepthBufferPortal& depthBuffer) const
  {
    viskores::Id x0 = static_cast<viskores::Id>(viskores::Round(start[0]));
    viskores::Id y0 = static_cast<viskores::Id>(viskores::Round(start[1]));
    viskores::Float32 z0 = static_cast<viskores::Float32>(start[2]);
    viskores::Id x1 = static_cast<viskores::Id>(viskores::Round(end[0]));
    viskores::Id y1 = static_cast<viskores::Id>(viskores::Round(end[1]));
    viskores::Float32 z1 = static_cast<viskores::Float32>(end[2]);
    viskores::Id dx = viskores::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    viskores::Id dy = -viskores::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    viskores::Id err = dx + dy, err2 = 0;

    const viskores::Id xStart = x0;
    const viskores::Id yStart = y0;
    const viskores::Float32 pdist =
      viskores::Sqrt(viskores::Float32(dx * dx) + viskores::Float32(dy * dy));

    while (x0 >= 0 && x0 < this->Width && y0 >= 0 && y0 < this->Height)
    {
      viskores::Float32 deltaX = static_cast<viskores::Float32>(x0 - xStart);
      viskores::Float32 deltaY = static_cast<viskores::Float32>(y0 - yStart);
      // Depth is wrong, but its far less wrong that it used to be.
      // These depth values are in screen space, which have been
      // potentially tranformed by a perspective correction.
      // To interpolated the depth correctly, there must be a perspective correction.
      // I haven't looked, but the wireframmer probably suffers from this too.
      // Additionally, this should not happen on the CPU. Annotations take
      // far longer than the the geometry.
      viskores::Float32 t =
        pdist == 0.f ? 1.0f : viskores::Sqrt(deltaX * deltaX + deltaY * deltaY) / pdist;
      t = viskores::Min(1.f, viskores::Max(0.f, t));
      viskores::Float32 z = viskores::Lerp(z0, z1, t);

      viskores::Id index = y0 * this->Width + x0;
      viskores::Vec4f_32 currentColor = colorBuffer.Get(index);
      viskores::Float32 currentZ = depthBuffer.Get(index);
      bool blend = currentColor[3] < 1.f && z > currentZ;
      if (currentZ > z || blend)
      {
        viskores::Vec4f_32 writeColor = color;
        viskores::Float32 depth = z;

        if (blend)
        {
          // If there is any transparency, all alphas
          // have been pre-mulitplied
          viskores::Float32 alpha = (1.f - currentColor[3]);
          writeColor[0] = currentColor[0] + color[0] * alpha;
          writeColor[1] = currentColor[1] + color[1] * alpha;
          writeColor[2] = currentColor[2] + color[2] * alpha;
          writeColor[3] = 1.f * alpha + currentColor[3]; // we are always drawing opaque lines
          // keep the current z. Line z interpolation is not accurate
          // Matt: this is correct. Interpolation is wrong
          depth = currentZ;
        }

        depthBuffer.Set(index, depth);
        colorBuffer.Set(index, writeColor);
      }

      if (x0 == x1 && y0 == y1)
      {
        break;
      }
      err2 = err * 2;
      if (err2 >= dy)
      {
        err += dy;
        x0 += sx;
      }
      if (err2 <= dx)
      {
        err += dx;
        y0 += sy;
      }
    }
  }

  viskores::Id Width;
  viskores::Id Height;
}; // struct RenderLine
} // namespace

LineRendererBatcher::LineRendererBatcher() {}

void LineRendererBatcher::BatchLine(const viskores::Vec3f_64& start,
                                    const viskores::Vec3f_64& end,
                                    const viskores::rendering::Color& color)
{
  viskores::Vec3f_32 start32(static_cast<viskores::Float32>(start[0]),
                             static_cast<viskores::Float32>(start[1]),
                             static_cast<viskores::Float32>(start[2]));
  viskores::Vec3f_32 end32(static_cast<viskores::Float32>(end[0]),
                           static_cast<viskores::Float32>(end[1]),
                           static_cast<viskores::Float32>(end[2]));
  this->BatchLine(start32, end32, color);
}

void LineRendererBatcher::BatchLine(const viskores::Vec3f_32& start,
                                    const viskores::Vec3f_32& end,
                                    const viskores::rendering::Color& color)
{
  this->Starts.push_back(start);
  this->Ends.push_back(end);
  this->Colors.push_back(color.Components);
}

void LineRendererBatcher::Render(const viskores::rendering::Canvas* canvas) const
{
  PointsArrayHandle starts =
    viskores::cont::make_ArrayHandle(this->Starts, viskores::CopyFlag::Off);
  PointsArrayHandle ends = viskores::cont::make_ArrayHandle(this->Ends, viskores::CopyFlag::Off);
  ColorsArrayHandle colors =
    viskores::cont::make_ArrayHandle(this->Colors, viskores::CopyFlag::Off);

  viskores::cont::Invoker invoker;
  invoker(RenderLine(canvas->GetWidth(), canvas->GetHeight()),
          starts,
          ends,
          colors,
          canvas->GetColorBuffer(),
          canvas->GetDepthBuffer());
}
}
} // namespace viskores::rendering
