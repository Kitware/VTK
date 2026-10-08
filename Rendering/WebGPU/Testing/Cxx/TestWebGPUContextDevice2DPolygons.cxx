// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContextDevice2DPolygons.cxx
 * @brief This test Solid polygon, Colored polygon, Quad strip, Convex polygon, and Ellipses
 * using WebGPUContext2D
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

    // Solid Polygon
    const std::array<float, 6> trianglePoints = { 50.0f, 250.0f, 150.0f, 250.0f, 100.0f, 350.0f };
    context->GetPen()->SetColor(0, 102, 204, 255); // Blue
    device->DrawPoly(const_cast<float*>(trianglePoints.data()), 3);
    drawLabel(100.0f, 230.0f, "Solid Polygon");

    // Colored Polygon
    const std::array<float, 12> quadPoints = { 250.0f, 250.0f, 350.0f, 250.0f, 250.0f, 350.0f,
      250.0f, 350.0f, 350.0f, 250.0f, 350.0f, 350.0f };
    const std::array<unsigned char, 24> vertexColors = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255,
      255, 0, 0, 255, 255, 0, 255, 0, 255, 255, 255, 0, 255 };
    device->DrawPoly(
      const_cast<float*>(quadPoints.data()), 6, const_cast<unsigned char*>(vertexColors.data()), 4);
    drawLabel(300.0f, 230.0f, "Colored Polygon");

    // Quad Strip
    context->GetPen()->SetColor(0, 102, 204, 255); // Blue
    const std::array<float, 12> quadStrip = { 30.0f, 60.0f, 80.0f, 60.0f, 30.0f, 110.0f, 80.0f,
      110.0f, 50.0f, 160.0f, 100.0f, 160.0f };
    device->DrawQuadStrip(const_cast<float*>(quadStrip.data()), 6);
    drawLabel(65.0f, 30.0f, "Quad Strip");

    // Convex Polygon
    context->GetPen()->SetColor(0, 255, 100, 255); // Green
    const std::array<float, 10> pentagon = { 200.0f, 70.0f, 240.0f, 100.0f, 220.0f, 160.0f, 180.0f,
      160.0f, 160.0f, 100.0f };
    device->DrawPolygon(const_cast<float*>(pentagon.data()), 5);
    drawLabel(200.0f, 30.0f, "Convex Polygon");

    // Ellipses
    context->GetPen()->SetColor(255, 255, 0, 255); // Yellow
    device->DrawEllipseWedge(330.0f, 110.0f, 35.0f, 35.0f, 0.0f, 0.0f, 30.0f, 330.0f);

    context->GetPen()->SetWidth(2.0f);
    context->GetPen()->SetColor(255, 255, 255, 255); // White
    device->DrawEllipticArc(330.0f, 110.0f, 35.0f, 35.0f, 30.0f, 330.0f);
    drawLabel(330.0f, 30.0f, "Ellipse Arc");

    return true;
  }
};

}

vtkStandardNewMacro(vtkWebGPUContextTestItem);

int TestWebGPUContextDevice2DPolygons(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetWindowName("WebGPU Context Device 2D Polygons Test");

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
