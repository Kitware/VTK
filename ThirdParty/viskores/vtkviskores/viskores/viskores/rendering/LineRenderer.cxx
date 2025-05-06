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

#include <viskores/rendering/LineRenderer.h>

#include <viskores/Transform3D.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/rendering/LineRendererBatcher.h>

namespace viskores
{
namespace rendering
{

LineRenderer::LineRenderer(const viskores::rendering::Canvas* canvas,
                           viskores::Matrix<viskores::Float32, 4, 4> transform,
                           viskores::rendering::LineRendererBatcher* lineBatcher)
  : Canvas(canvas)
  , Transform(transform)
  , LineBatcher(lineBatcher)
{
}

void LineRenderer::RenderLine(const viskores::Vec2f_64& point0,
                              const viskores::Vec2f_64& point1,
                              viskores::Float32 lineWidth,
                              const viskores::rendering::Color& color)
{
  RenderLine(viskores::make_Vec(point0[0], point0[1], 0.0),
             viskores::make_Vec(point1[0], point1[1], 0.0),
             lineWidth,
             color);
}

void LineRenderer::RenderLine(const viskores::Vec3f_64& point0,
                              const viskores::Vec3f_64& point1,
                              viskores::Float32 viskoresNotUsed(lineWidth),
                              const viskores::rendering::Color& color)
{
  viskores::Vec3f_32 p0 = TransformPoint(point0);
  viskores::Vec3f_32 p1 = TransformPoint(point1);
  this->LineBatcher->BatchLine(p0, p1, color);
}

viskores::Vec3f_32 LineRenderer::TransformPoint(const viskores::Vec3f_64& point) const
{
  viskores::Vec4f_32 temp(static_cast<viskores::Float32>(point[0]),
                          static_cast<viskores::Float32>(point[1]),
                          static_cast<viskores::Float32>(point[2]),
                          1.0f);
  temp = viskores::MatrixMultiply(Transform, temp);
  viskores::Vec3f_32 p;
  for (viskores::IdComponent i = 0; i < 3; ++i)
  {
    p[i] = static_cast<viskores::Float32>(temp[i] / temp[3]);
  }
  p[0] = (p[0] * 0.5f + 0.5f) * static_cast<viskores::Float32>(Canvas->GetWidth());
  p[1] = (p[1] * 0.5f + 0.5f) * static_cast<viskores::Float32>(Canvas->GetHeight());
  p[2] = (p[2] * 0.5f + 0.5f) - 0.001f;
  return p;
}
}
} // namespace viskores::rendering
