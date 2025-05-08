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
#ifndef viskores_rendering_ColorLegendAnnotation_h
#define viskores_rendering_ColorLegendAnnotation_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/TextAnnotationScreen.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT ColorLegendAnnotation
{
private:
  viskores::Float32 FontScale;
  viskores::rendering::Color LabelColor;
  std::vector<std::string> Labels;
  std::vector<std::unique_ptr<TextAnnotationScreen>> Annot;
  std::vector<viskores::rendering::Color> ColorSwatchList;

public:
  ColorLegendAnnotation();
  ColorLegendAnnotation(const ColorLegendAnnotation&) = delete;
  ColorLegendAnnotation& operator=(const ColorLegendAnnotation&) = delete;

  void Clear();
  void AddItem(const std::string& label, viskores::rendering::Color color);

  void SetLabelColor(viskores::rendering::Color c) { this->LabelColor = c; }

  void SetLabelFontScale(viskores::Float32 s)
  {
    this->FontScale = s;
    for (auto& annot : this->Annot)
      annot->SetScale(s);
  }

  void Render(const viskores::rendering::Camera&,
              const viskores::rendering::WorldAnnotator& annotator,
              viskores::rendering::Canvas& canvas);
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_ColorLegendAnnotation_h
