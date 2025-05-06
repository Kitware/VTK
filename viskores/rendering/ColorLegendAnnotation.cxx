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

#include <viskores/rendering/ColorLegendAnnotation.h>

namespace viskores
{
namespace rendering
{

ColorLegendAnnotation::ColorLegendAnnotation()
{
  this->FontScale = 0.05f;
  this->LabelColor = viskores::rendering::Color::white;
}

void ColorLegendAnnotation::Clear()
{
  this->Labels.clear();
  this->ColorSwatchList.clear();
}

void ColorLegendAnnotation::AddItem(const std::string& label, viskores::rendering::Color color)
{
  this->Labels.push_back(label);
  this->ColorSwatchList.push_back(color);
}

void ColorLegendAnnotation::Render(const viskores::rendering::Camera& camera,
                                   const viskores::rendering::WorldAnnotator& annotator,
                                   viskores::rendering::Canvas& canvas)
{
  viskores::Float32 l = -0.95f, r = -0.90f;
  viskores::Float32 b = +0.90f, t = +0.95f;

  for (auto& color : this->ColorSwatchList)
  {
    canvas.AddColorSwatch(l, b, l, t, r, t, r, b, color);
    b -= 0.07f;
    t -= 0.07f;
  }

  // reset positions
  l = -0.95f;
  r = -0.90f;
  b = +0.90f;
  t = +0.95f;

  while (this->Annot.size() < this->Labels.size())
  {
    this->Annot.push_back(std::make_unique<TextAnnotationScreen>(
      "test", this->LabelColor, this->FontScale, viskores::Vec2f_32(0, 0)));
  }

  for (unsigned int i = 0; i < this->Annot.size(); ++i)
  {
    TextAnnotationScreen* txt = Annot[i].get();
    txt->SetText(Labels[i]);
    txt->SetPosition(r + .02f, (b + t) / 2.f);
    txt->SetAlignment(TextAnnotationScreen::Left, TextAnnotationScreen::VCenter);
    txt->Render(camera, annotator, canvas);
    b -= 0.07f;
    t -= 0.07f;
  }
}
}
} // namespace viskores::rendering
