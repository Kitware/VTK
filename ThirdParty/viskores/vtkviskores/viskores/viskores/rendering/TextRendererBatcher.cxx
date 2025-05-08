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

#include <viskores/rendering/TextRendererBatcher.h>

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace rendering
{
namespace
{

struct RenderBitmapFont : public viskores::worklet::WorkletMapField
{
  using ColorBufferType = viskores::rendering::Canvas::ColorBufferType;
  using DepthBufferType = viskores::rendering::Canvas::DepthBufferType;
  using FontTextureType = viskores::rendering::Canvas::FontTextureType;

  using ControlSignature =
    void(FieldIn, FieldIn, FieldIn, FieldIn, ExecObject, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  VISKORES_CONT
  RenderBitmapFont() {}

  VISKORES_CONT
  RenderBitmapFont(viskores::Id width, viskores::Id height)
    : Width(width)
    , Height(height)
  {
  }

  template <typename ColorBufferPortal, typename FontTexture, typename DepthBufferPortal>
  VISKORES_EXEC void operator()(const viskores::Vec4f_32& screenCoords,
                                const viskores::Vec4f_32& textureCoords,
                                const viskores::Vec4f_32& color,
                                const viskores::Float32& depth,
                                const FontTexture& fontTexture,
                                ColorBufferPortal& colorBuffer,
                                DepthBufferPortal& depthBuffer) const
  {
    viskores::Float32 x0 = Clamp(screenCoords[0], 0.0f, static_cast<viskores::Float32>(Width - 1));
    viskores::Float32 x1 = Clamp(screenCoords[2], 0.0f, static_cast<viskores::Float32>(Width - 1));
    viskores::Float32 y0 = Clamp(screenCoords[1], 0.0f, static_cast<viskores::Float32>(Height - 1));
    viskores::Float32 y1 = Clamp(screenCoords[3], 0.0f, static_cast<viskores::Float32>(Height - 1));
    // For crisp text rendering, we sample the font texture at points smaller than the pixel
    // sizes. Here we sample at increments of 0.25f, and scale the reported intensities accordingly
    viskores::Float32 dx = x1 - x0, dy = y1 - y0;
    for (viskores::Float32 x = x0; x <= x1; x += 0.25f)
    {
      for (viskores::Float32 y = y0; y <= y1; y += 0.25f)
      {
        viskores::Float32 tu = x1 == x0 ? 1.0f : (x - x0) / dx;
        viskores::Float32 tv = y1 == y0 ? 1.0f : (y - y0) / dy;
        viskores::Float32 u = viskores::Lerp(textureCoords[0], textureCoords[2], tu);
        viskores::Float32 v = viskores::Lerp(textureCoords[1], textureCoords[3], tv);
        viskores::Float32 intensity = fontTexture.GetColor(u, v)[0] * 0.25f;
        Plot(x, y, intensity, color, depth, colorBuffer, depthBuffer);
      }
    }
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VISKORES_EXEC void Plot(viskores::Float32 x,
                          viskores::Float32 y,
                          viskores::Float32 intensity,
                          viskores::Vec4f_32 color,
                          viskores::Float32 depth,
                          ColorBufferPortal& colorBuffer,
                          DepthBufferPortal& depthBuffer) const
  {
    viskores::Id index = static_cast<viskores::Id>(viskores::Round(y)) * Width +
      static_cast<viskores::Id>(viskores::Round(x));
    viskores::Vec4f_32 srcColor = colorBuffer.Get(index);
    viskores::Float32 currentDepth = depthBuffer.Get(index);
    bool swap = depth > currentDepth;

    intensity = intensity * color[3];
    color = intensity * color;
    color[3] = intensity;
    viskores::Vec4f_32 front = color;
    viskores::Vec4f_32 back = srcColor;

    if (swap)
    {
      front = srcColor;
      back = color;
    }

    viskores::Vec4f_32 blendedColor;
    viskores::Float32 alpha = (1.f - front[3]);
    blendedColor[0] = front[0] + back[0] * alpha;
    blendedColor[1] = front[1] + back[1] * alpha;
    blendedColor[2] = front[2] + back[2] * alpha;
    blendedColor[3] = back[3] * alpha + front[3];

    colorBuffer.Set(index, blendedColor);
  }

  VISKORES_EXEC
  viskores::Float32 Clamp(viskores::Float32 v, viskores::Float32 min, viskores::Float32 max) const
  {
    return viskores::Min(viskores::Max(v, min), max);
  }

  viskores::Id Width;
  viskores::Id Height;
}; // struct RenderBitmapFont
} // namespace

TextRendererBatcher::TextRendererBatcher(
  const viskores::rendering::Canvas::FontTextureType& fontTexture)
  : FontTexture(fontTexture)
{
}

void TextRendererBatcher::BatchText(const ScreenCoordsArrayHandle& screenCoords,
                                    const TextureCoordsArrayHandle& textureCoords,
                                    const viskores::rendering::Color& color,
                                    const viskores::Float32& depth)
{
  viskores::Id textLength = screenCoords.GetNumberOfValues();
  ScreenCoordsArrayHandle::ReadPortalType screenCoordsP = screenCoords.ReadPortal();
  TextureCoordsArrayHandle::ReadPortalType textureCoordsP = textureCoords.ReadPortal();
  for (int i = 0; i < textLength; ++i)
  {
    this->ScreenCoords.push_back(screenCoordsP.Get(i));
    this->TextureCoords.push_back(textureCoordsP.Get(i));
    this->Colors.push_back(color.Components);
    this->Depths.push_back(depth);
  }
}

void TextRendererBatcher::Render(const viskores::rendering::Canvas* canvas) const
{
  ScreenCoordsArrayHandle screenCoords =
    viskores::cont::make_ArrayHandle(this->ScreenCoords, viskores::CopyFlag::Off);
  TextureCoordsArrayHandle textureCoords =
    viskores::cont::make_ArrayHandle(this->TextureCoords, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<ColorType> colors =
    viskores::cont::make_ArrayHandle(this->Colors, viskores::CopyFlag::Off);
  viskores::cont::ArrayHandle<viskores::Float32> depths =
    viskores::cont::make_ArrayHandle(this->Depths, viskores::CopyFlag::Off);

  viskores::cont::Invoker invoker;
  invoker(RenderBitmapFont(canvas->GetWidth(), canvas->GetHeight()),
          screenCoords,
          textureCoords,
          colors,
          depths,
          this->FontTexture.GetExecObjectFactory(),
          canvas->GetColorBuffer(),
          canvas->GetDepthBuffer());
}
}
} // namespace viskores::rendering
