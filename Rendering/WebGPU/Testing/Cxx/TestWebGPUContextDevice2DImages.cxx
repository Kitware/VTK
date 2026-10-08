// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestWebGPUContextDevice2DImages.cxx
 * @brief This test drawing multiple RGBA images and polydata with WebGPUContext2D
 */

#include "vtkCellArray.h"
#include "vtkContext2D.h"
#include "vtkContextActor.h"
#include "vtkContextItem.h"
#include "vtkContextScene.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSetGet.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTextProperty.h"
#include "vtkWebGPUContextDevice2D.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

namespace
{

class vtkWebGPUContextTestItem : public vtkContextItem
{
public:
  static vtkWebGPUContextTestItem* New();
  vtkTypeMacro(vtkWebGPUContextTestItem, vtkContextItem);

  vtkSmartPointer<vtkImageData> Image1;
  vtkSmartPointer<vtkImageData> Image2;

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

    // PolyData Mesh
    vtkNew<vtkPolyData> poly;
    vtkNew<vtkPoints> pts;
    pts->InsertNextPoint(0, 0, 0);
    pts->InsertNextPoint(100, 0, 0);
    pts->InsertNextPoint(50, 100, 0);
    poly->SetPoints(pts);

    vtkNew<vtkCellArray> polys;
    polys->InsertNextCell({ 0, 1, 2 });
    poly->SetPolys(polys);

    float offset[2] = { 50.0f, 150.0f };
    context->GetPen()->SetColor(255, 255, 0, 255); // Yellow
    device->DrawPolyData(offset, 1.0f, poly, nullptr, 0);
    drawLabel(100.0f, 120.0f, "PolyData Mesh");

    if (this->Image1)
    {
      context->DrawImage(vtkRectf(230.0f, 220.0f, 120.0f, 120.0f), this->Image1);
      drawLabel(290.0f, 190.0f, "Texture Image 1");
    }

    if (this->Image2)
    {
      context->DrawImage(vtkRectf(230.0f, 40.0f, 120.0f, 120.0f), this->Image2);
      drawLabel(290.0f, 10.0f, "Texture Transparent Image 2");
    }

    return true;
  }
};

}

vtkStandardNewMacro(vtkWebGPUContextTestItem);

int TestWebGPUContextDevice2DImages(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  char* file1 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk.png");
  vtkNew<vtkPNGReader> reader1;
  reader1->SetFileName(file1);
  reader1->Update();
  delete[] file1;

  char* file2 = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk-transparent.png");
  vtkNew<vtkPNGReader> reader2;
  reader2->SetFileName(file2);
  reader2->Update();
  delete[] file2;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetWindowName("WebGPU Context 2D Device Images Test");

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.15, 0.15, 0.15);
  renWin->AddRenderer(renderer);

  vtkNew<vtkContextActor> contextActor;

  vtkNew<vtkWebGPUContextTestItem> testItem;

  if (reader1->GetOutput()->GetPointData()->GetScalars())
  {
    testItem->Image1 = reader1->GetOutput();
  }
  else
  {
    vtkLogF(ERROR, "Failed to load first test image.");
  }

  if (reader2->GetOutput()->GetPointData()->GetScalars())
  {
    testItem->Image2 = reader2->GetOutput();
  }
  else
  {
    vtkLogF(ERROR, "Failed to load second test image.");
  }

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
