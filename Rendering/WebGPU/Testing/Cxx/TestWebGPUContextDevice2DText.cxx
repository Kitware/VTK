// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContextDevice2DText.cxx
 * @brief This test DrawString text and Math text of WebGPUContext2D
 */

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkMathTextUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
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

    // Text
    context->GetTextProp()->SetColor(0.0, 1.0, 1.0);
    context->GetTextProp()->SetFontSize(28);
    context->GetTextProp()->SetJustificationToCentered();
    context->GetTextProp()->SetVerticalJustificationToCentered();
    context->DrawString(200.0f, 280.0f, "Kitware CHAMPION!");
    drawLabel(200.0f, 230.0f, "DrawString Text");

    // Math Text
    context->GetTextProp()->SetColor(1.0, 1.0, 1.0);
    context->GetTextProp()->SetFontSize(30);
    const std::array<float, 2> mathOffset = { 200.0f, 150.0f };
    device->DrawMathTextString(const_cast<float*>(mathOffset.data()), "$A = \\pi r^2$");
    drawLabel(200.0f, 100.0f, "DrawMathTextString");

    return true;
  }
};

}

vtkStandardNewMacro(vtkWebGPUContextTestItem);

int TestWebGPUContextDevice2DText(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetWindowName("WebGPU Context Device 2D Text Test");

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
