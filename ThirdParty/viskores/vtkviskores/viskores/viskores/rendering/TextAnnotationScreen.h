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
#ifndef viskores_rendering_TextAnnotationScreen_h
#define viskores_rendering_TextAnnotationScreen_h

#include <viskores/rendering/TextAnnotation.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT TextAnnotationScreen : public TextAnnotation
{
protected:
  viskores::Vec2f_32 Position;
  viskores::Float32 Angle;

public:
  TextAnnotationScreen(const std::string& text,
                       const viskores::rendering::Color& color,
                       viskores::Float32 scale,
                       const viskores::Vec2f_32& position,
                       viskores::Float32 angleDegrees = 0);

  void SetPosition(const viskores::Vec2f_32& position);

  void SetPosition(viskores::Float32 posx, viskores::Float32 posy);

  void Render(const viskores::rendering::Camera& camera,
              const viskores::rendering::WorldAnnotator& annotator,
              viskores::rendering::Canvas& canvas) const override;
};
}
} // namespace viskores::rendering

#endif //viskores_rendering_TextAnnotationScreen_h
