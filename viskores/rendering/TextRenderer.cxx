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

#include <viskores/rendering/TextRenderer.h>

#include <viskores/Transform3D.h>
#include <viskores/cont/TryExecute.h>
#include <viskores/rendering/TextRendererBatcher.h>

namespace viskores
{
namespace rendering
{

TextRenderer::TextRenderer(const viskores::rendering::Canvas* canvas,
                           const viskores::rendering::BitmapFont& font,
                           const viskores::rendering::Canvas::FontTextureType& fontTexture,
                           viskores::rendering::TextRendererBatcher* textBatcher)
  : Canvas(canvas)
  , Font(font)
  , FontTexture(fontTexture)
  , TextBatcher(textBatcher)
{
}

void TextRenderer::RenderText(const viskores::Vec2f_32& position,
                              viskores::Float32 scale,
                              viskores::Float32 angle,
                              viskores::Float32 windowAspect,
                              const viskores::Vec2f_32& anchor,
                              const viskores::rendering::Color& color,
                              const std::string& text)
{
  viskores::Matrix<viskores::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  viskores::Matrix<viskores::Float32, 4, 4> scaleMatrix =
    Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  viskores::Vec3f_32 rotationAxis(0.0f, 0.0f, 1.0f);
  viskores::Matrix<viskores::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  viskores::Matrix<viskores::Float32, 4, 4> transform = viskores::MatrixMultiply(
    translationMatrix, viskores::MatrixMultiply(scaleMatrix, rotationMatrix));
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const viskores::Vec3f_32& origin,
                              const viskores::Vec3f_32& right,
                              const viskores::Vec3f_32& up,
                              viskores::Float32 scale,
                              const viskores::Vec2f_32& anchor,
                              const viskores::rendering::Color& color,
                              const std::string& text)
{
  viskores::Vec3f_32 n = viskores::Cross(right, up);
  viskores::Normalize(n);

  viskores::Matrix<viskores::Float32, 4, 4> transform =
    MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = viskores::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = viskores::MatrixMultiply(Canvas->GetProjection(), transform);
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const viskores::Matrix<viskores::Float32, 4, 4>& transform,
                              viskores::Float32 scale,
                              const viskores::Vec2f_32& anchor,
                              const viskores::rendering::Color& color,
                              const std::string& text,
                              const viskores::Float32& depth)
{
  viskores::Float32 textWidth = this->Font.GetTextWidth(text);
  viskores::Float32 fx = -(0.5f + 0.5f * anchor[0]) * textWidth;
  viskores::Float32 fy = -(0.5f + 0.5f * anchor[1]);
  viskores::Float32 fz = 0;

  using ScreenCoordsArrayHandle = viskores::cont::ArrayHandle<viskores::Id4>;
  using TextureCoordsArrayHandle = viskores::cont::ArrayHandle<viskores::Vec4f_32>;
  ScreenCoordsArrayHandle screenCoords;
  TextureCoordsArrayHandle textureCoords;
  {
    screenCoords.Allocate(static_cast<viskores::Id>(text.length()));
    textureCoords.Allocate(static_cast<viskores::Id>(text.length()));
    ScreenCoordsArrayHandle::WritePortalType screenCoordsPortal = screenCoords.WritePortal();
    TextureCoordsArrayHandle::WritePortalType textureCoordsPortal = textureCoords.WritePortal();
    viskores::Vec4f_32 charVertices, charUVs, charCoords;
    for (std::size_t i = 0; i < text.length(); ++i)
    {
      char c = text[i];
      char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;
      Font.GetCharPolygon(c,
                          fx,
                          fy,
                          charVertices[0],
                          charVertices[2],
                          charVertices[3],
                          charVertices[1],
                          charUVs[0],
                          charUVs[2],
                          charUVs[3],
                          charUVs[1],
                          nextchar);
      charVertices = charVertices * scale;
      viskores::Id2 p0 = Canvas->GetScreenPoint(charVertices[0], charVertices[3], fz, transform);
      viskores::Id2 p1 = Canvas->GetScreenPoint(charVertices[2], charVertices[1], fz, transform);
      charCoords = viskores::Id4(p0[0], p1[1], p1[0], p0[1]);
      screenCoordsPortal.Set(static_cast<viskores::Id>(i), charCoords);
      textureCoordsPortal.Set(static_cast<viskores::Id>(i), charUVs);
    }
  }

  this->TextBatcher->BatchText(screenCoords, textureCoords, color, depth);
}
}
} // namespace viskores::rendering
