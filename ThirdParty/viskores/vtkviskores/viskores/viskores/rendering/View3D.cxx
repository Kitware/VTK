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

#include <viskores/rendering/View3D.h>

namespace viskores
{
namespace rendering
{

View3D::View3D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, backgroundColor, foregroundColor)
{
}

View3D::View3D(const viskores::rendering::Scene& scene,
               const viskores::rendering::Mapper& mapper,
               const viskores::rendering::Canvas& canvas,
               const viskores::rendering::Camera& camera,
               const viskores::rendering::Color& backgroundColor,
               const viskores::rendering::Color& foregroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor, foregroundColor)
{
}

void View3D::Paint()
{
  this->GetCanvas().Clear();
  this->RenderAnnotations();
  this->GetScene().Render(this->GetMapper(), this->GetCanvas(), this->GetCamera());
}

void View3D::RenderScreenAnnotations()
{
  if (this->GetScene().GetNumberOfActors() > 0)
  {
    this->GetCanvas().BeginTextRenderingBatch();
    this->GetWorldAnnotator().BeginLineRenderingBatch();
    //this->ColorBarAnnotation.SetAxisColor(viskores::rendering::Color(1,1,1));
    this->ColorBarAnnotation.SetFieldName(this->GetScene().GetActor(0).GetScalarField().GetName());
    this->ColorBarAnnotation.SetRange(this->GetScene().GetActor(0).GetScalarRange(), 5);
    this->ColorBarAnnotation.SetColorTable(this->GetScene().GetActor(0).GetColorTable());
    this->ColorBarAnnotation.Render(
      this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
    this->GetWorldAnnotator().EndLineRenderingBatch();
    this->GetCanvas().EndTextRenderingBatch();
  }
}

void View3D::RenderWorldAnnotations()
{
  this->GetCanvas().BeginTextRenderingBatch();
  viskores::Bounds bounds = this->GetScene().GetSpatialBounds();
  viskores::Float64 xmin = bounds.X.Min, xmax = bounds.X.Max;
  viskores::Float64 ymin = bounds.Y.Min, ymax = bounds.Y.Max;
  viskores::Float64 zmin = bounds.Z.Min, zmax = bounds.Z.Max;
  viskores::Float64 dx = xmax - xmin, dy = ymax - ymin, dz = zmax - zmin;
  viskores::Float64 size = viskores::Sqrt(dx * dx + dy * dy + dz * dz);

  this->GetWorldAnnotator().BeginLineRenderingBatch();
  this->BoxAnnotation.SetColor(Color(.5f, .5f, .5f));
  this->BoxAnnotation.SetExtents(this->GetScene().GetSpatialBounds());
  this->BoxAnnotation.Render(this->GetCamera(), this->GetWorldAnnotator());
  this->GetWorldAnnotator().EndLineRenderingBatch();

  viskores::Vec3f_32 lookAt = this->GetCamera().GetLookAt();
  viskores::Vec3f_32 position = this->GetCamera().GetPosition();
  bool xtest = lookAt[0] > position[0];
  bool ytest = lookAt[1] > position[1];
  bool ztest = lookAt[2] > position[2];

  const bool outsideedges = true; // if false, do closesttriad
  if (outsideedges)
  {
    xtest = !xtest;
    //ytest = !ytest;
  }

  viskores::Float64 xrel = viskores::Abs(dx) / size;
  viskores::Float64 yrel = viskores::Abs(dy) / size;
  viskores::Float64 zrel = viskores::Abs(dz) / size;

  this->GetWorldAnnotator().BeginLineRenderingBatch();
  this->XAxisAnnotation.SetAxis(0);
  this->XAxisAnnotation.SetColor(AxisColor);
  this->XAxisAnnotation.SetTickInvert(xtest, ytest, ztest);
  this->XAxisAnnotation.SetWorldPosition(
    xmin, ytest ? ymin : ymax, ztest ? zmin : zmax, xmax, ytest ? ymin : ymax, ztest ? zmin : zmax);
  this->XAxisAnnotation.SetRange(xmin, xmax);
  this->XAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
  this->XAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
  this->XAxisAnnotation.SetLabelFontOffset(viskores::Float32(size / 15.f));
  this->XAxisAnnotation.SetMoreOrLessTickAdjustment(xrel < .3 ? -1 : 0);
  this->XAxisAnnotation.Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  this->YAxisAnnotation.SetAxis(1);
  this->YAxisAnnotation.SetColor(AxisColor);
  this->YAxisAnnotation.SetTickInvert(xtest, ytest, ztest);
  this->YAxisAnnotation.SetWorldPosition(
    xtest ? xmin : xmax, ymin, ztest ? zmin : zmax, xtest ? xmin : xmax, ymax, ztest ? zmin : zmax);
  this->YAxisAnnotation.SetRange(ymin, ymax);
  this->YAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
  this->YAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
  this->YAxisAnnotation.SetLabelFontOffset(viskores::Float32(size / 15.f));
  this->YAxisAnnotation.SetMoreOrLessTickAdjustment(yrel < .3 ? -1 : 0);
  this->YAxisAnnotation.Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  this->ZAxisAnnotation.SetAxis(2);
  this->ZAxisAnnotation.SetColor(AxisColor);
  this->ZAxisAnnotation.SetTickInvert(xtest, ytest, ztest);
  this->ZAxisAnnotation.SetWorldPosition(
    xtest ? xmin : xmax, ytest ? ymin : ymax, zmin, xtest ? xmin : xmax, ytest ? ymin : ymax, zmax);
  this->ZAxisAnnotation.SetRange(zmin, zmax);
  this->ZAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
  this->ZAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
  this->ZAxisAnnotation.SetLabelFontOffset(viskores::Float32(size / 15.f));
  this->ZAxisAnnotation.SetMoreOrLessTickAdjustment(zrel < .3 ? -1 : 0);
  this->ZAxisAnnotation.Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
  this->GetWorldAnnotator().EndLineRenderingBatch();

  this->GetCanvas().EndTextRenderingBatch();
}
}
} // namespace viskores::rendering
