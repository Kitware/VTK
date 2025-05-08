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

#include <viskores/rendering/Color.h>
#include <viskores/rendering/TextAnnotation.h>
#include <viskores/rendering/View1D.h>

namespace viskores
{
namespace rendering
{

View1D::View1D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, backgroundColor, foregroundColor)
{
}

View1D::View1D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Camera& camera,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor, foregroundColor)
{
}

void View1D::Paint()
{
  this->GetCanvas().Clear();
  this->UpdateCameraProperties();
  this->AddAdditionalAnnotation([&]() { this->RenderColorLegendAnnotations(); });
  this->RenderAnnotations();
  this->GetScene().Render(this->GetMapper(), this->GetCanvas(), this->GetCamera());
}

void View1D::RenderScreenAnnotations()
{
  viskores::Float32 viewportLeft;
  viskores::Float32 viewportRight;
  viskores::Float32 viewportTop;
  viskores::Float32 viewportBottom;
  this->GetCamera().GetRealViewport(this->GetCanvas().GetWidth(),
                                    this->GetCanvas().GetHeight(),
                                    viewportLeft,
                                    viewportRight,
                                    viewportBottom,
                                    viewportTop);

  this->GetCanvas().BeginTextRenderingBatch();
  this->GetWorldAnnotator().BeginLineRenderingBatch();
  this->HorizontalAxisAnnotation.SetColor(AxisColor);
  this->HorizontalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportRight, viewportBottom);
  viskores::Bounds viewRange = this->GetCamera().GetViewRange2D();

  this->HorizontalAxisAnnotation.SetLogarithmic(LogX);
  this->HorizontalAxisAnnotation.SetRangeForAutoTicks(viewRange.X.Min, viewRange.X.Max);
  this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
  this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
  this->HorizontalAxisAnnotation.SetLabelAlignment(viskores::rendering::TextAnnotation::HCenter,
                                                   viskores::rendering::TextAnnotation::Top);
  this->HorizontalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  viskores::Float32 windowaspect = viskores::Float32(this->GetCanvas().GetWidth()) /
    viskores::Float32(this->GetCanvas().GetHeight());

  this->VerticalAxisAnnotation.SetColor(AxisColor);
  this->VerticalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportLeft, viewportTop);
  this->VerticalAxisAnnotation.SetLogarithmic(LogY);
  this->VerticalAxisAnnotation.SetRangeForAutoTicks(viewRange.Y.Min, viewRange.Y.Max);
  this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetLabelAlignment(viskores::rendering::TextAnnotation::Right,
                                                 viskores::rendering::TextAnnotation::VCenter);
  this->VerticalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  this->GetWorldAnnotator().EndLineRenderingBatch();
  this->GetCanvas().EndTextRenderingBatch();
}

void View1D::RenderColorLegendAnnotations()
{
  if (LegendEnabled)
  {
    this->Legend.Clear();
    this->GetWorldAnnotator().BeginLineRenderingBatch();
    this->GetCanvas().BeginTextRenderingBatch();
    for (int i = 0; i < this->GetScene().GetNumberOfActors(); ++i)
    {
      const auto& act = this->GetScene().GetActor(i);

      viskores::Vec<double, 4> colorData;
      act.GetColorTable().GetPoint(0, colorData);

      //colorData[0] is the transfer function x position
      viskores::rendering::Color color{ static_cast<viskores::Float32>(colorData[1]),
                                        static_cast<viskores::Float32>(colorData[2]),
                                        static_cast<viskores::Float32>(colorData[3]) };
      this->Legend.AddItem(act.GetScalarField().GetName(), color);
    }
    this->Legend.SetLabelColor(this->GetCanvas().GetForegroundColor());
    this->Legend.Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
    this->GetWorldAnnotator().EndLineRenderingBatch();
    this->GetCanvas().EndTextRenderingBatch();
  }
}

void View1D::RenderWorldAnnotations()
{
  // 1D views don't have world annotations.
}

void View1D::EnableLegend()
{
  LegendEnabled = true;
}

void View1D::DisableLegend()
{
  LegendEnabled = false;
}

void View1D::UpdateCameraProperties()
{
  // Modify the camera if we are going log scaling or if our bounds are equal
  viskores::Bounds origCamBounds = this->GetCamera().GetViewRange2D();
  viskores::Float64 vmin = origCamBounds.Y.Min;
  viskores::Float64 vmax = origCamBounds.Y.Max;
  if (LogY)
  {
    if (vmin <= 0 || vmax <= 0)
    {
      origCamBounds.Y.Min = 0;
      origCamBounds.Y.Max = 1;
    }
    else
    {
      origCamBounds.Y.Min = log10(vmin);
      origCamBounds.Y.Max = log10(vmax);
      if (origCamBounds.Y.Min == origCamBounds.Y.Max)
      {
        origCamBounds.Y.Min /= 10;
        origCamBounds.Y.Max *= 10;
      }
    }
  }
  else
  {
    origCamBounds.Y.Min = vmin;
    origCamBounds.Y.Max = vmax;
    if (origCamBounds.Y.Min == origCamBounds.Y.Max)
    {
      origCamBounds.Y.Min -= .5;
      origCamBounds.Y.Max += .5;
    }
  }

  // Set camera bounds with new top/bottom values
  this->GetCamera().SetViewRange2D(
    origCamBounds.X.Min, origCamBounds.X.Max, origCamBounds.Y.Min, origCamBounds.Y.Max);

  // if unchanged by user we always want to start with a curve being full-frame
  if (this->GetCamera().GetMode() == Camera::Mode::TwoD && this->GetCamera().GetXScale() == 1.0f)
  {
    viskores::Float32 left, right, bottom, top;
    this->GetCamera().GetViewRange2D(left, right, bottom, top);
    this->GetCamera().SetXScale((static_cast<viskores::Float32>(this->GetCanvas().GetWidth())) /
                                (static_cast<viskores::Float32>(this->GetCanvas().GetHeight())) *
                                (top - bottom) / (right - left));
  }
}
}
} // namespace viskores::rendering
