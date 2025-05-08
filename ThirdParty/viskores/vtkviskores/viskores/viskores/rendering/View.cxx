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

#include <viskores/rendering/View.h>

namespace viskores
{
namespace rendering
{

struct View::InternalData
{
  ~InternalData()
  {
    delete MapperPointer;
    delete CanvasPointer;
    delete WorldAnnotatorPointer;
  }
  viskores::rendering::Scene Scene;
  viskores::rendering::Mapper* MapperPointer{ nullptr };
  viskores::rendering::Canvas* CanvasPointer{ nullptr };
  viskores::rendering::WorldAnnotator* WorldAnnotatorPointer{ nullptr };
  std::vector<std::unique_ptr<viskores::rendering::TextAnnotation>> TextAnnotations;
  std::vector<std::function<void(void)>> AdditionalAnnotations;
  viskores::rendering::Camera Camera;
};

View::View(const viskores::rendering::Scene& scene,
           const viskores::rendering::Mapper& mapper,
           const viskores::rendering::Canvas& canvas,
           const viskores::rendering::Color& backgroundColor,
           const viskores::rendering::Color& foregroundColor)
  : Internal(std::make_unique<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;

  viskores::Bounds spatialBounds = this->Internal->Scene.GetSpatialBounds();
  this->Internal->Camera.ResetToBounds(spatialBounds);
  if (spatialBounds.Z.Length() > 0.0)
  {
    this->Internal->Camera.SetModeTo3D();
  }
  else
  {
    this->Internal->Camera.SetModeTo2D();
  }
}

View::View(const viskores::rendering::Scene& scene,
           const viskores::rendering::Mapper& mapper,
           const viskores::rendering::Canvas& canvas,
           const viskores::rendering::Camera& camera,
           const viskores::rendering::Color& backgroundColor,
           const viskores::rendering::Color& foregroundColor)
  : Internal(std::make_unique<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->Camera = camera;
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;
}

View::~View() = default;

const viskores::rendering::Scene& View::GetScene() const
{
  return this->Internal->Scene;
}

viskores::rendering::Scene& View::GetScene()
{
  return this->Internal->Scene;
}

void View::SetScene(const viskores::rendering::Scene& scene)
{
  this->Internal->Scene = scene;
}

const viskores::rendering::Mapper& View::GetMapper() const
{
  return *this->Internal->MapperPointer;
}

viskores::rendering::Mapper& View::GetMapper()
{
  return *this->Internal->MapperPointer;
}

const viskores::rendering::Canvas& View::GetCanvas() const
{
  return *this->Internal->CanvasPointer;
}

viskores::rendering::Canvas& View::GetCanvas()
{
  return *this->Internal->CanvasPointer;
}

const viskores::rendering::WorldAnnotator& View::GetWorldAnnotator() const
{
  return *this->Internal->WorldAnnotatorPointer;
}

const viskores::rendering::Camera& View::GetCamera() const
{
  return this->Internal->Camera;
}

viskores::rendering::Camera& View::GetCamera()
{
  return this->Internal->Camera;
}

void View::SetCamera(const viskores::rendering::Camera& camera)
{
  this->Internal->Camera = camera;
}

const viskores::rendering::Color& View::GetBackgroundColor() const
{
  return this->Internal->CanvasPointer->GetBackgroundColor();
}

void View::SetBackgroundColor(const viskores::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetBackgroundColor(color);
}

void View::SetForegroundColor(const viskores::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetForegroundColor(color);
}

void View::SaveAs(const std::string& fileName) const
{
  this->GetCanvas().SaveAs(fileName);
}

void View::SetAxisColor(viskores::rendering::Color c)
{
  this->AxisColor = c;
}

void View::ClearTextAnnotations()
{
  this->Internal->TextAnnotations.clear();
}

void View::AddTextAnnotation(std::unique_ptr<viskores::rendering::TextAnnotation> ann)
{
  this->Internal->TextAnnotations.push_back(std::move(ann));
}

void View::ClearAdditionalAnnotations()
{
  this->Internal->AdditionalAnnotations.clear();
}

void View::AddAdditionalAnnotation(std::function<void(void)> ann)
{
  this->Internal->AdditionalAnnotations.emplace_back(ann);
}

void View::RenderAnnotations()
{
  if (this->RenderAnnotationsEnabled)
  {
    this->SetupForScreenSpace();
    this->RenderScreenAnnotations();

    this->GetCanvas().BeginTextRenderingBatch();
    for (auto& textAnnotation : this->Internal->TextAnnotations)
    {
      textAnnotation->Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
    }
    this->GetCanvas().EndTextRenderingBatch();

    for (auto& additionalAnnotation : this->Internal->AdditionalAnnotations)
    {
      additionalAnnotation();
    }

    this->SetupForWorldSpace();
    if (this->WorldAnnotationsEnabled)
    {
      this->RenderWorldAnnotations();
    }
  }
}

void View::SetupForWorldSpace(bool viewportClip)
{
  this->GetCanvas().SetViewToWorldSpace(this->Internal->Camera, viewportClip);
}

void View::SetupForScreenSpace(bool viewportClip)
{
  this->GetCanvas().SetViewToScreenSpace(this->Internal->Camera, viewportClip);
}

} // namespace viskores::rendering
} // namespace viskores
