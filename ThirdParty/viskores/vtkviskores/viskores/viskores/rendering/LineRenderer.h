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

#ifndef viskores_rendering_LineRenderer_h
#define viskores_rendering_LineRenderer_h

#include <viskores/Matrix.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
class LineRendererBatcher;

class VISKORES_RENDERING_EXPORT LineRenderer
{
public:
  VISKORES_CONT
  LineRenderer(const viskores::rendering::Canvas* canvas,
               viskores::Matrix<viskores::Float32, 4, 4> transform,
               viskores::rendering::LineRendererBatcher* lineBatcher);

  VISKORES_CONT
  void RenderLine(const viskores::Vec2f_64& point0,
                  const viskores::Vec2f_64& point1,
                  viskores::Float32 lineWidth,
                  const viskores::rendering::Color& color);

  VISKORES_CONT
  void RenderLine(const viskores::Vec3f_64& point0,
                  const viskores::Vec3f_64& point1,
                  viskores::Float32 lineWidth,
                  const viskores::rendering::Color& color);

private:
  VISKORES_CONT
  viskores::Vec3f_32 TransformPoint(const viskores::Vec3f_64& point) const;

  const viskores::rendering::Canvas* Canvas;
  viskores::Matrix<viskores::Float32, 4, 4> Transform;
  viskores::rendering::LineRendererBatcher* LineBatcher;
}; // class LineRenderer
}
} // namespace viskores::rendering

#endif // viskores_rendering_LineRenderer_h
