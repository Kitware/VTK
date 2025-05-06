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
#ifndef viskores_rendering_TextAnnotation_h
#define viskores_rendering_TextAnnotation_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT TextAnnotation
{
public:
  enum struct HorizontalAlignment
  {
    Left,
    HCenter,
    Right
  };
  enum struct VerticalAlignment
  {
    Bottom,
    VCenter,
    Top
  };
  static constexpr HorizontalAlignment Left = HorizontalAlignment::Left;
  static constexpr HorizontalAlignment HCenter = HorizontalAlignment::HCenter;
  static constexpr HorizontalAlignment Right = HorizontalAlignment::Right;
  static constexpr VerticalAlignment Bottom = VerticalAlignment::Bottom;
  static constexpr VerticalAlignment VCenter = VerticalAlignment::VCenter;
  static constexpr VerticalAlignment Top = VerticalAlignment::Top;

protected:
  std::string Text;
  Color TextColor;
  viskores::Float32 Scale;
  viskores::Vec2f_32 Anchor;

public:
  TextAnnotation(const std::string& text,
                 const viskores::rendering::Color& color,
                 viskores::Float32 scalar);

  virtual ~TextAnnotation();

  void SetText(const std::string& text);

  const std::string& GetText() const;

  /// Set the anchor point relative to the box containing the text. The anchor
  /// is scaled in both directions to the range [-1,1] with -1 at the lower
  /// left and 1 at the upper right.
  ///
  void SetRawAnchor(const viskores::Vec2f_32& anchor);

  void SetRawAnchor(viskores::Float32 h, viskores::Float32 v);

  void SetAlignment(HorizontalAlignment h, VerticalAlignment v);

  void SetScale(viskores::Float32 scale);

  virtual void Render(const viskores::rendering::Camera& camera,
                      const viskores::rendering::WorldAnnotator& worldAnnotator,
                      viskores::rendering::Canvas& canvas) const = 0;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_TextAnnotation_h
