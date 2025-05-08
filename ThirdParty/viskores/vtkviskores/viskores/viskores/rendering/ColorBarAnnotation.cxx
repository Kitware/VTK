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

#include <viskores/rendering/ColorBarAnnotation.h>
#include <viskores/rendering/TextAnnotationScreen.h>

namespace viskores
{
namespace rendering
{

ColorBarAnnotation::ColorBarAnnotation()
  : ColorTable(viskores::ColorSpace::Lab)
  , Position(viskores::Range(-0.88, +0.88), viskores::Range(+0.87, +0.92), viskores::Range(0, 0))
  , Horizontal(true)
{
}

void ColorBarAnnotation::SetFieldName(const std::string& fieldName)
{
  FieldName = fieldName;
}

void ColorBarAnnotation::SetPosition(const viskores::Bounds& position)
{
  Position = position;
  viskores::Float64 x = Position.X.Length();
  viskores::Float64 y = Position.Y.Length();
  if (x > y)
    Horizontal = true;
  else
    Horizontal = false;
}

void ColorBarAnnotation::SetRange(const viskores::Range& range, viskores::IdComponent numTicks)
{
  std::vector<viskores::Float64> positions, proportions;
  this->Axis.SetMinorTicks(positions, proportions); // clear any minor ticks

  for (viskores::IdComponent i = 0; i < numTicks; ++i)
  {
    viskores::Float64 prop =
      static_cast<viskores::Float64>(i) / static_cast<viskores::Float64>(numTicks - 1);
    viskores::Float64 pos = range.Min + prop * range.Length();
    positions.push_back(pos);
    proportions.push_back(prop);
  }
  this->Axis.SetMajorTicks(positions, proportions);
}

void ColorBarAnnotation::Render(const viskores::rendering::Camera& camera,
                                const viskores::rendering::WorldAnnotator& worldAnnotator,
                                viskores::rendering::Canvas& canvas)
{

  canvas.AddColorBar(Position, this->ColorTable, Horizontal);

  this->Axis.SetColor(canvas.GetForegroundColor());
  this->Axis.SetLineWidth(1);

  if (Horizontal)
  {
    this->Axis.SetScreenPosition(Position.X.Min, Position.Y.Min, Position.X.Max, Position.Y.Min);
    this->Axis.SetLabelAlignment(TextAnnotation::HCenter, TextAnnotation::Top);
    this->Axis.SetMajorTickSize(0, .02, 1.0);
  }
  else
  {
    this->Axis.SetScreenPosition(Position.X.Min, Position.Y.Min, Position.X.Min, Position.Y.Max);
    this->Axis.SetLabelAlignment(TextAnnotation::Right, TextAnnotation::VCenter);
    this->Axis.SetMajorTickSize(.02, 0.0, 1.0);
  }

  this->Axis.SetMinorTickSize(0, 0, 0); // no minor ticks
  this->Axis.Render(camera, worldAnnotator, canvas);

  if (!FieldName.empty())
  {
    viskores::Vec2f_32 labelPos;
    if (Horizontal)
    {
      labelPos[0] = viskores::Float32(Position.X.Min);
      labelPos[1] = viskores::Float32(Position.Y.Max);
    }
    else
    {
      labelPos[0] = viskores::Float32(Position.X.Min - 0.07);
      labelPos[1] = viskores::Float32(Position.Y.Max + 0.03);
    }

    viskores::rendering::TextAnnotationScreen var(FieldName,
                                                  canvas.GetForegroundColor(),
                                                  .045f, // font scale
                                                  labelPos,
                                                  0.f); // rotation

    var.Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace viskores::rendering
