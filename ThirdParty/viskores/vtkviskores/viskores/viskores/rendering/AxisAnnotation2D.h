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
#ifndef viskores_rendering_AxisAnnotation2D_h
#define viskores_rendering_AxisAnnotation2D_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/Range.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/AxisAnnotation.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>
#include <viskores/rendering/TextAnnotation.h>
#include <viskores/rendering/WorldAnnotator.h>

namespace viskores
{
namespace rendering
{

class VISKORES_RENDERING_EXPORT AxisAnnotation2D : public AxisAnnotation
{
protected:
  viskores::Float64 MajorTickSizeX, MajorTickSizeY, MajorTickOffset;
  viskores::Float64 MinorTickSizeX, MinorTickSizeY, MinorTickOffset;
  viskores::Float64 PosX0, PosY0, PosX1, PosY1;
  viskores::Range TickRange;
  viskores::Float32 FontScale;
  viskores::Float32 LineWidth;
  viskores::rendering::Color Color;
  bool Logarithmic;

  TextAnnotation::HorizontalAlignment AlignH;
  TextAnnotation::VerticalAlignment AlignV;
  std::vector<std::unique_ptr<TextAnnotation>> Labels;

  std::vector<viskores::Float64> PositionsMajor;
  std::vector<viskores::Float64> ProportionsMajor;

  std::vector<viskores::Float64> PositionsMinor;
  std::vector<viskores::Float64> ProportionsMinor;

  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation2D();

  AxisAnnotation2D(const AxisAnnotation2D&) = delete;

  AxisAnnotation2D& operator=(const AxisAnnotation2D&) = delete;

  void SetLogarithmic(bool l) { this->Logarithmic = l; }

  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  void SetColor(viskores::rendering::Color c) { this->Color = c; }

  void SetLineWidth(viskores::Float32 lw) { this->LineWidth = lw; }

  void SetMajorTickSize(viskores::Float64 xlen, viskores::Float64 ylen, viskores::Float64 offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    this->MajorTickSizeX = xlen;
    this->MajorTickSizeY = ylen;
    this->MajorTickOffset = offset;
  }

  void SetMinorTickSize(viskores::Float64 xlen, viskores::Float64 ylen, viskores::Float64 offset)
  {
    this->MinorTickSizeX = xlen;
    this->MinorTickSizeY = ylen;
    this->MinorTickOffset = offset;
  }

  ///\todo: rename, since it might be screen OR world position?
  void SetScreenPosition(viskores::Float64 x0,
                         viskores::Float64 y0,
                         viskores::Float64 x1,
                         viskores::Float64 y1)
  {
    this->PosX0 = x0;
    this->PosY0 = y0;

    this->PosX1 = x1;
    this->PosY1 = y1;
  }

  void SetLabelAlignment(TextAnnotation::HorizontalAlignment h, TextAnnotation::VerticalAlignment v)
  {
    this->AlignH = h;
    this->AlignV = v;
  }

  void SetLabelFontScale(viskores::Float32 s)
  {
    this->FontScale = s;
    for (auto& label : this->Labels)
      label->SetScale(s);
  }

  void SetRangeForAutoTicks(const viskores::Range& range);
  void SetRangeForAutoTicks(viskores::Float64 lower, viskores::Float64 upper)
  {
    this->SetRangeForAutoTicks(viskores::Range(lower, upper));
  }

  void SetMajorTicks(const std::vector<viskores::Float64>& positions,
                     const std::vector<viskores::Float64>& proportions);

  void SetMinorTicks(const std::vector<viskores::Float64>& positions,
                     const std::vector<viskores::Float64>& proportions);

  void Render(const viskores::rendering::Camera& camera,
              const viskores::rendering::WorldAnnotator& worldAnnotator,
              viskores::rendering::Canvas& canvas) override;
};
}
} //namespace viskores::rendering

#endif // viskores_rendering_AxisAnnotation2D_h
