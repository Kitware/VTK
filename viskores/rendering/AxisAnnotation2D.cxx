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

#include <viskores/rendering/AxisAnnotation2D.h>

#include <viskores/rendering/TextAnnotationScreen.h>

#include <sstream>

namespace viskores
{
namespace rendering
{

AxisAnnotation2D::AxisAnnotation2D()
{
  this->AlignH = TextAnnotation::HCenter;
  this->AlignV = TextAnnotation::VCenter;
  this->FontScale = 0.05f;
  this->LineWidth = 1.0;
  this->Color = viskores::rendering::Color(1, 1, 1);
  this->Logarithmic = false;
  this->MoreOrLessTickAdjustment = 0;
}

void AxisAnnotation2D::SetRangeForAutoTicks(const Range& range)
{
  this->TickRange = range;

  if (this->Logarithmic)
  {
    CalculateTicksLogarithmic(this->TickRange, false, this->PositionsMajor, this->ProportionsMajor);
    CalculateTicksLogarithmic(this->TickRange, true, this->PositionsMinor, this->ProportionsMinor);
  }
  else
  {
    CalculateTicks(this->TickRange,
                   false,
                   this->PositionsMajor,
                   this->ProportionsMajor,
                   this->MoreOrLessTickAdjustment);
    CalculateTicks(this->TickRange,
                   true,
                   this->PositionsMinor,
                   this->ProportionsMinor,
                   this->MoreOrLessTickAdjustment);
  }
}

void AxisAnnotation2D::SetMajorTicks(const std::vector<viskores::Float64>& pos,
                                     const std::vector<viskores::Float64>& prop)
{
  this->PositionsMajor.clear();
  this->PositionsMajor.insert(this->PositionsMajor.begin(), pos.begin(), pos.end());

  this->ProportionsMajor.clear();
  this->ProportionsMajor.insert(this->ProportionsMajor.begin(), prop.begin(), prop.end());
}

void AxisAnnotation2D::SetMinorTicks(const std::vector<viskores::Float64>& pos,
                                     const std::vector<viskores::Float64>& prop)
{
  this->PositionsMinor.clear();
  this->PositionsMinor.insert(this->PositionsMinor.begin(), pos.begin(), pos.end());

  this->ProportionsMinor.clear();
  this->ProportionsMinor.insert(this->ProportionsMinor.begin(), prop.begin(), prop.end());
}

void AxisAnnotation2D::Render(const viskores::rendering::Camera& camera,
                              const viskores::rendering::WorldAnnotator& worldAnnotator,
                              viskores::rendering::Canvas& canvas)
{
  canvas.AddLine(this->PosX0, this->PosY0, this->PosX1, this->PosY1, this->LineWidth, this->Color);

  // major ticks
  auto nmajor = (unsigned int)this->ProportionsMajor.size();
  while (this->Labels.size() < nmajor)
  {
    this->Labels.push_back(std::make_unique<TextAnnotationScreen>(
      "test", this->Color, this->FontScale, viskores::Vec2f_32(0, 0)));
  }

  std::stringstream numberToString;
  for (unsigned int i = 0; i < nmajor; ++i)
  {
    viskores::Float64 xc = this->PosX0 + (this->PosX1 - this->PosX0) * this->ProportionsMajor[i];
    viskores::Float64 yc = this->PosY0 + (this->PosY1 - this->PosY0) * this->ProportionsMajor[i];
    viskores::Float64 xs = xc - this->MajorTickSizeX * this->MajorTickOffset;
    viskores::Float64 xe = xc + this->MajorTickSizeX * (1. - this->MajorTickOffset);
    viskores::Float64 ys = yc - this->MajorTickSizeY * this->MajorTickOffset;
    viskores::Float64 ye = yc + this->MajorTickSizeY * (1. - this->MajorTickOffset);

    canvas.AddLine(xs, ys, xe, ye, 1.0, this->Color);

    if (this->MajorTickSizeY == 0)
    {
      // slight shift to space between label and tick
      xs -= (this->MajorTickSizeX < 0 ? -1. : +1.) * this->FontScale * .1;
    }

    numberToString.str("");
    numberToString << this->PositionsMajor[i];

    this->Labels[i]->SetText(numberToString.str());
    //if (fabs(this->PositionsMajor[i]) < 1e-10)
    //    this->Labels[i]->SetText("0");
    auto* tempDerived = dynamic_cast<TextAnnotationScreen*>(this->Labels[i].get());
    tempDerived->SetPosition(viskores::Float32(xs), viskores::Float32(ys));

    this->Labels[i]->SetAlignment(this->AlignH, this->AlignV);
  }

  // minor ticks
  if (this->MinorTickSizeX != 0 || this->MinorTickSizeY != 0)
  {
    auto nminor = (unsigned int)this->ProportionsMinor.size();
    for (unsigned int i = 0; i < nminor; ++i)
    {
      viskores::Float64 xc = this->PosX0 + (this->PosX1 - this->PosX0) * this->ProportionsMinor[i];
      viskores::Float64 yc = this->PosY0 + (this->PosY1 - this->PosY0) * this->ProportionsMinor[i];
      viskores::Float64 xs = xc - this->MinorTickSizeX * this->MinorTickOffset;
      viskores::Float64 xe = xc + this->MinorTickSizeX * (1. - this->MinorTickOffset);
      viskores::Float64 ys = yc - this->MinorTickSizeY * this->MinorTickOffset;
      viskores::Float64 ye = yc + this->MinorTickSizeY * (1. - this->MinorTickOffset);

      canvas.AddLine(xs, ys, xe, ye, 1.0, this->Color);
    }
  }

  for (auto& label : this->Labels)
  {
    label->Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace viskores::rendering
