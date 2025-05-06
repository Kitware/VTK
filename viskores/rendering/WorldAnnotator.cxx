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
#include <viskores/rendering/TextRenderer.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

WorldAnnotator::WorldAnnotator(const viskores::rendering::Canvas* canvas)
  : Canvas(canvas)
{
}

void WorldAnnotator::AddLine(const viskores::Vec3f_64& point0,
                             const viskores::Vec3f_64& point1,
                             viskores::Float32 lineWidth,
                             const viskores::rendering::Color& color,
                             bool viskoresNotUsed(inFront)) const
{
  viskores::Matrix<viskores::Float32, 4, 4> transform =
    viskores::MatrixMultiply(Canvas->GetProjection(), Canvas->GetModelView());
  LineRenderer renderer(Canvas, transform, &(this->LineBatcher));
  renderer.RenderLine(point0, point1, lineWidth, color);
}

void WorldAnnotator::BeginLineRenderingBatch() const
{
  this->LineBatcher = viskores::rendering::LineRendererBatcher();
}

void WorldAnnotator::EndLineRenderingBatch() const
{
  this->LineBatcher.Render(this->Canvas);
}

void WorldAnnotator::AddText(const viskores::Vec3f_32& origin,
                             const viskores::Vec3f_32& right,
                             const viskores::Vec3f_32& up,
                             viskores::Float32 scale,
                             const viskores::Vec2f_32& anchor,
                             const viskores::rendering::Color& color,
                             const std::string& text,
                             viskores::Float32 depth) const
{
  viskores::Vec3f_32 n = viskores::Cross(right, up);
  viskores::Normalize(n);

  viskores::Matrix<viskores::Float32, 4, 4> transform =
    MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = viskores::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = viskores::MatrixMultiply(Canvas->GetProjection(), transform);
  Canvas->AddText(transform, scale, anchor, color, text, depth);
}
}
} // namespace viskores::rendering
