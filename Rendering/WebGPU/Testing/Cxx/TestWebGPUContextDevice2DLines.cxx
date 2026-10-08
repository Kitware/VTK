// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContextDevice2DLines.cxx
 * @brief This test thin lines, thick lines and thick points of WebGPUContext2D
 */

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSetGet.h"
#include "vtkTesting.h"
#include "vtkTextProperty.h"
#include "vtkWebGPUContextDevice2D.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <array>

namespace
{

class vtkWebGPUContextTestItem : public vtkContextItem
{
public:
  static vtkWebGPUContextTestItem* New();
  vtkTypeMacro(vtkWebGPUContextTestItem, vtkContextItem);

  bool Paint(vtkContext2D* context) override
  {
    vtkContextDevice2D* device = context->GetDevice();

    auto drawLabel = [&](float x, float y, const std::string& text)
    {
      context->GetTextProp()->SetColor(1.0, 1.0, 1.0); // White
      context->GetTextProp()->SetFontSize(14);
      context->GetTextProp()->SetJustificationToCentered();
      context->GetTextProp()->SetVerticalJustificationToTop();
      context->DrawString(x, y, text);
    };

    // Thin Lines
    context->GetPen()->SetWidth(1.0f);
    context->GetPen()->SetColor(255, 165, 0, 255); // Orange
    const std::array<float, 8> thinLineSegments = { 50.0f, 250.0f, 80.0f, 350.0f, 120.0f, 250.0f,
      150.0f, 350.0f };
    device->DrawLines(const_cast<float*>(thinLineSegments.data()), 4);
    drawLabel(100.0f, 220.0f, "Thin Lines");

    // Thick Lines
    context->GetPen()->SetWidth(10.0f);
    context->GetPen()->SetColor(255, 0, 255, 255); // Magenta
    const std::array<float, 8> thickLineSegments = { 250.0f, 250.0f, 280.0f, 350.0f, 320.0f, 250.0f,
      350.0f, 350.0f };
    device->DrawLines(const_cast<float*>(thickLineSegments.data()), 4);
    drawLabel(300.0f, 220.0f, "Thick Lines");

    // Thick Points
    context->GetPen()->SetWidth(16.0f);
    context->GetPen()->SetColor(0, 255, 255, 255); // Cyan
    const std::array<float, 6> testPoints = { 150.0f, 100.0f, 200.0f, 100.0f, 250.0f, 100.0f };
    device->DrawPoints(const_cast<float*>(testPoints.data()), 3);
    drawLabel(200.0f, 70.0f, "Thick Points");

    return true;
  }
};

}

vtkStandardNewMacro(vtkWebGPUContextTestItem);

int TestWebGPUContextDevice2DLines(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetWindowName("WebGPU Context Device 2D Lines Test");

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.15, 0.15, 0.15);
  renWin->AddRenderer(renderer);

  vtkNew<vtkContextActor> contextActor;

  vtkNew<vtkWebGPUContextTestItem> testItem;

  contextActor->GetScene()->AddItem(testItem);
  renderer->AddActor(contextActor);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renWin);
  renWin->Initialize();
  renWin->Render();

  if (testing->IsInteractiveModeSpecified())
  {
    interactor->Start();
  }

  int retVal = vtkRegressionTestImage(renWin);

  return !retVal;
}
