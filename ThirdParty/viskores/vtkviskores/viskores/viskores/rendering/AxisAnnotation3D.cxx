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

#include <viskores/rendering/AxisAnnotation3D.h>

#include <sstream>

namespace viskores
{
namespace rendering
{

AxisAnnotation3D::AxisAnnotation3D()
  : AxisAnnotation()
  , TickMajorSize(1.0)
  , TickMajorOffset(1.0)
  , TickMinorSize(0.1)
  , TickMinorOffset(1.0)
  , Axis(0)
  , Invert(1, 1, 1)
  , Point0(0, 0, 0)
  , Point1(1, 0, 0)
  , Range(0, 1)
  , FontScale(0.05f)
  , // screen space font size
  FontOffset(0.1f)
  , // world space offset from axis
  LineWidth(1.0)
  , Color(1, 1, 1)
  , MoreOrLessTickAdjustment(0)
{
}

void AxisAnnotation3D::SetTickInvert(bool x, bool y, bool z)
{
  this->Invert[0] = x ? +1.0f : -1.0f;
  this->Invert[1] = y ? +1.0f : -1.0f;
  this->Invert[2] = z ? +1.0f : -1.0f;
}

void AxisAnnotation3D::SetLabelFontScale(Float64 s)
{
  this->FontScale = s;
  for (auto& label : this->Labels)
  {
    label->SetScale(viskores::Float32(s));
  }
}

void AxisAnnotation3D::Render(const Camera& camera,
                              const WorldAnnotator& worldAnnotator,
                              Canvas& canvas)
{
  bool infront = true;
  worldAnnotator.AddLine(this->Point0, this->Point1, this->LineWidth, this->Color, infront);

  std::vector<viskores::Float64> positions;
  std::vector<viskores::Float64> proportions;
  // major ticks
  CalculateTicks(this->Range, false, positions, proportions, this->MoreOrLessTickAdjustment);
  auto nmajor = (unsigned int)proportions.size();
  while (this->Labels.size() < nmajor)
  {
    this->Labels.push_back(std::make_unique<TextAnnotationBillboard>(
      "test", this->Color, viskores::Float32(this->FontScale), viskores::Vec3f_32(0, 0, 0)));
  }

  std::stringstream numberToString;
  for (unsigned int i = 0; i < nmajor; ++i)
  {
    viskores::Vec3f_64 tickPos = proportions[i] * (this->Point1 - this->Point0) + this->Point0;
    for (int pass = 0; pass <= 1; pass++)
    {
      viskores::Vec3f_64 tickSize(0);
      if (pass == 0)
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[1] = this->TickMajorSize;
            break;
          case 1:
            tickSize[0] = this->TickMajorSize;
            break;
          case 2:
            tickSize[0] = this->TickMajorSize;
            break;
        }
      }
      else // pass == 1
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[2] = this->TickMajorSize;
            break;
          case 1:
            tickSize[2] = this->TickMajorSize;
            break;
          case 2:
            tickSize[1] = this->TickMajorSize;
            break;
        }
      }
      tickSize = tickSize * this->Invert;
      viskores::Vec3f_64 start = tickPos - tickSize * this->TickMajorOffset;
      viskores::Vec3f_64 end = tickPos - tickSize * (1.0 - this->TickMajorOffset);

      worldAnnotator.AddLine(start, end, this->LineWidth, this->Color, infront);
    }

    viskores::Vec3f_32 tickSize(0);
    viskores::Float32 s = 0.4f * this->FontOffset;
    switch (this->Axis)
    {
      case 0:
        tickSize[1] = s;
        tickSize[2] = s;
        break;
      case 1:
        tickSize[0] = s;
        tickSize[2] = s;
        break;
      case 2:
        tickSize[0] = s;
        tickSize[1] = s;
        break;
    }
    tickSize = tickSize * this->Invert;

    numberToString.str("");
    numberToString << positions[i];
    this->Labels[i]->SetText(numberToString.str());
    //if (fabs(positions[i]) < 1e-10)
    //    this->Labels[i]->SetText("0");
    this->Labels[i]->SetPosition(viskores::Float32(tickPos[0] - tickSize[0]),
                                 viskores::Float32(tickPos[1] - tickSize[1]),
                                 viskores::Float32(tickPos[2] - tickSize[2]));
    this->Labels[i]->SetAlignment(TextAnnotation::HCenter, TextAnnotation::VCenter);
  }

  // minor ticks
  CalculateTicks(this->Range, true, positions, proportions, this->MoreOrLessTickAdjustment);
  auto nminor = (unsigned int)proportions.size();
  for (unsigned int i = 0; i < nminor; ++i)
  {
    viskores::Vec3f_64 tickPos = proportions[i] * (this->Point1 - this->Point0) + this->Point0;
    for (int pass = 0; pass <= 1; pass++)
    {
      viskores::Vec3f_64 tickSize(0);
      if (pass == 0)
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[1] = this->TickMinorSize;
            break;
          case 1:
            tickSize[0] = this->TickMinorSize;
            break;
          case 2:
            tickSize[0] = this->TickMinorSize;
            break;
        }
      }
      else // pass == 1
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[2] = this->TickMinorSize;
            break;
          case 1:
            tickSize[2] = this->TickMinorSize;
            break;
          case 2:
            tickSize[1] = this->TickMinorSize;
            break;
        }
      }
      tickSize = tickSize * this->Invert;
      viskores::Vec3f_64 start = tickPos - tickSize * this->TickMinorOffset;
      viskores::Vec3f_64 end = tickPos - tickSize * (1.0 - this->TickMinorOffset);

      worldAnnotator.AddLine(start, end, this->LineWidth, this->Color, infront);
    }
  }

  for (auto& label : this->Labels)
  {
    label->Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace viskores::rendering
