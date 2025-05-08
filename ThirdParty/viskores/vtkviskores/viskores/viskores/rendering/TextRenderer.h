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

#ifndef viskores_rendering_TextRenderer_h
#define viskores_rendering_TextRenderer_h

#include <string>

#include <viskores/rendering/BitmapFont.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{
class TextRendererBatcher;

class VISKORES_RENDERING_EXPORT TextRenderer
{
public:
  VISKORES_CONT
  TextRenderer(const viskores::rendering::Canvas* canvas,
               const viskores::rendering::BitmapFont& font,
               const viskores::rendering::Canvas::FontTextureType& fontTexture,
               viskores::rendering::TextRendererBatcher* textBatcher);

  VISKORES_CONT
  void RenderText(const viskores::Vec2f_32& position,
                  viskores::Float32 scale,
                  viskores::Float32 angle,
                  viskores::Float32 windowAspect,
                  const viskores::Vec2f_32& anchor,
                  const viskores::rendering::Color& color,
                  const std::string& text);

  VISKORES_CONT
  void RenderText(const viskores::Vec3f_32& origin,
                  const viskores::Vec3f_32& right,
                  const viskores::Vec3f_32& up,
                  viskores::Float32 scale,
                  const viskores::Vec2f_32& anchor,
                  const viskores::rendering::Color& color,
                  const std::string& text);

  VISKORES_CONT
  void RenderText(const viskores::Matrix<viskores::Float32, 4, 4>& transform,
                  viskores::Float32 scale,
                  const viskores::Vec2f_32& anchor,
                  const viskores::rendering::Color& color,
                  const std::string& text,
                  const viskores::Float32& depth = 0.f);

private:
  const viskores::rendering::Canvas* Canvas;
  viskores::rendering::BitmapFont Font;
  viskores::rendering::Canvas::FontTextureType FontTexture;
  viskores::rendering::TextRendererBatcher* TextBatcher;
};
}
} // namespace viskores::rendering

#endif // viskores_rendering_TextRenderer_h
