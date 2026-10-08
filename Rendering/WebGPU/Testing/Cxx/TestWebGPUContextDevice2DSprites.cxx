// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContextDevice2DSprites.cxx
 * @brief This test Procedural image generation, Point sprites and Markers of
 * WebGPUContext2D
 */

#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkImageData.h"
#include "vtkMarkerUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSetGet.h"
#include "vtkTesting.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"
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

    // Procedural Image Generation
    vtkNew<vtkImageData> testImage;
    testImage->SetDimensions(64, 64, 1);
    testImage->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

    vtkUnsignedCharArray* scalars =
      vtkArrayDownCast<vtkUnsignedCharArray>(testImage->GetPointData()->GetScalars());

    scalars->FillComponent(0, 0);   // R
    scalars->FillComponent(1, 102); // G
    scalars->FillComponent(2, 204); // B
    scalars->FillComponent(3, 125); // A

    // Point Sprites
    context->GetPen()->SetWidth(48.0f);
    const std::array<float, 4> spritePoints = { 100.0f, 250.0f, 160.0f, 250.0f };
    device->DrawPointSprites(testImage, const_cast<float*>(spritePoints.data()), 2, nullptr, 0);
    drawLabel(130.0f, 180.0f, "Point Sprites");

    // Markers
    context->GetPen()->SetWidth(24.0f);
    context->GetPen()->SetColor(0, 102, 204, 255); // Blue
    const std::array<float, 6> markerPoints = { 240.0f, 250.0f, 280.0f, 250.0f, 320.0f, 250.0f };
    device->DrawMarkers(3, false, const_cast<float*>(markerPoints.data()), 3, nullptr, 0);
    drawLabel(280.0f, 180.0f, "Procedural Markers");

    return true;
  }
};

}

vtkStandardNewMacro(vtkWebGPUContextTestItem);

int TestWebGPUContextDevice2DSprites(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetWindowName("WebGPU Context 2D Device Sprites Test");

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
