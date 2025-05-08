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

#include <viskores/rendering/TextAnnotationScreen.h>

namespace viskores
{
namespace rendering
{

TextAnnotationScreen::TextAnnotationScreen(const std::string& text,
                                           const viskores::rendering::Color& color,
                                           viskores::Float32 scale,
                                           const viskores::Vec2f_32& position,
                                           viskores::Float32 angleDegrees)
  : TextAnnotation(text, color, scale)
  , Position(position)
  , Angle(angleDegrees)
{
}

void TextAnnotationScreen::SetPosition(const viskores::Vec2f_32& position)
{
  this->Position = position;
}

void TextAnnotationScreen::SetPosition(viskores::Float32 xpos, viskores::Float32 ypos)
{
  this->SetPosition(viskores::make_Vec(xpos, ypos));
}

void TextAnnotationScreen::Render(
  const viskores::rendering::Camera& viskoresNotUsed(camera),
  const viskores::rendering::WorldAnnotator& viskoresNotUsed(annotator),
  viskores::rendering::Canvas& canvas) const
{
  viskores::Float32 windowAspect =
    viskores::Float32(canvas.GetWidth()) / viskores::Float32(canvas.GetHeight());

  canvas.AddText(this->Position,
                 this->Scale,
                 this->Angle,
                 windowAspect,
                 this->Anchor,
                 this->TextColor,
                 this->Text);
}
}
} // namespace viskores::rendering
