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

#ifndef viskores_rendering_TextRendererBatcher_h
#define viskores_rendering_TextRendererBatcher_h

#include <string>
#include <vector>

#include <viskores/rendering/BitmapFont.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/viskores_rendering_export.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT TextRendererBatcher
{
public:
  using FontTextureType = viskores::rendering::Canvas::FontTextureType;
  using ScreenCoordsType = viskores::Id4;
  using TextureCoordsType = viskores::Vec4f_32;
  using ColorType = viskores::Vec4f_32;
  using ScreenCoordsArrayHandle = viskores::cont::ArrayHandle<ScreenCoordsType>;
  using TextureCoordsArrayHandle = viskores::cont::ArrayHandle<TextureCoordsType>;
  using ColorsArrayHandle = viskores::cont::ArrayHandle<ColorType>;
  using DepthsArrayHandle = viskores::cont::ArrayHandle<viskores::Float32>;

  /*
  VISKORES_CONT
  TextRendererBatcher();
  */

  VISKORES_CONT
  TextRendererBatcher(const viskores::rendering::Canvas::FontTextureType& fontTexture);

  VISKORES_CONT
  void BatchText(const ScreenCoordsArrayHandle& screenCoords,
                 const TextureCoordsArrayHandle& textureCoords,
                 const viskores::rendering::Color& color,
                 const viskores::Float32& depth);

  void Render(const viskores::rendering::Canvas* canvas) const;

private:
  viskores::rendering::Canvas::FontTextureType FontTexture;
  std::vector<ScreenCoordsType> ScreenCoords;
  std::vector<TextureCoordsType> TextureCoords;
  std::vector<ColorType> Colors;
  std::vector<viskores::Float32> Depths;
};
}
} // namespace viskores::rendering

#endif // viskores_rendering_TextRendererBatcher_h
