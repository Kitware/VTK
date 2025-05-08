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

#include <viskores/rendering/View2D.h>

namespace viskores
{
namespace rendering
{

View2D::View2D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, backgroundColor, foregroundColor)
{
}

View2D::View2D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Camera& camera,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor, foregroundColor)
{
}

void View2D::Paint()
{
  this->GetCanvas().Clear();
  this->UpdateCameraProperties();
  this->RenderAnnotations();
  this->GetScene().Render(this->GetMapper(), this->GetCanvas(), this->GetCamera());
}

void View2D::RenderScreenAnnotations()
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
  this->HorizontalAxisAnnotation.SetRangeForAutoTicks(viewRange.X.Min, viewRange.X.Max);
  this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
  this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
  this->HorizontalAxisAnnotation.SetLabelAlignment(TextAnnotation::HCenter, TextAnnotation::Top);
  this->HorizontalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  viskores::Float32 windowaspect = viskores::Float32(this->GetCanvas().GetWidth()) /
    viskores::Float32(this->GetCanvas().GetHeight());

  this->VerticalAxisAnnotation.SetColor(AxisColor);
  this->VerticalAxisAnnotation.SetScreenPosition(
    viewportLeft, viewportBottom, viewportLeft, viewportTop);
  this->VerticalAxisAnnotation.SetRangeForAutoTicks(viewRange.Y.Min, viewRange.Y.Max);
  this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetLabelAlignment(TextAnnotation::Right, TextAnnotation::VCenter);
  this->VerticalAxisAnnotation.Render(
    this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  const viskores::rendering::Scene& scene = this->GetScene();
  if (scene.GetNumberOfActors() > 0)
  {
    //this->ColorBarAnnotation.SetAxisColor(viskores::rendering::Color(1,1,1));
    this->ColorBarAnnotation.SetFieldName(scene.GetActor(0).GetScalarField().GetName());
    this->ColorBarAnnotation.SetRange(
      scene.GetActor(0).GetScalarRange().Min, scene.GetActor(0).GetScalarRange().Max, 5);
    this->ColorBarAnnotation.SetColorTable(scene.GetActor(0).GetColorTable());
    this->ColorBarAnnotation.Render(
      this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
  }
  this->GetWorldAnnotator().EndLineRenderingBatch();
  this->GetCanvas().EndTextRenderingBatch();
}

void View2D::RenderWorldAnnotations()
{
  // 2D views don't have world annotations.
}

void View2D::UpdateCameraProperties()
{
  // Modify the camera if our bounds are equal to enable an image to show
  viskores::Bounds origCamBounds = this->GetCamera().GetViewRange2D();
  if (origCamBounds.Y.Min == origCamBounds.Y.Max)
  {
    origCamBounds.Y.Min -= .5;
    origCamBounds.Y.Max += .5;
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
  viskores::Float32 left, right, bottom, top;
  this->GetCamera().GetViewRange2D(left, right, bottom, top);
}
}
} // namespace viskores::rendering
