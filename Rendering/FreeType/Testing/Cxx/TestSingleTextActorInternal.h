// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRegressionTestImage.h"
#include "vtkTesting.h"

#include "vtkProp.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"

#include <iostream>

namespace TestTextActor
{
inline std::string InputText()
{
  return "0123456789.";
}

inline int CreatePipeline(
  int argc, char* argv[], vtkProp* textActor, vtkTextProperty* textProperty, bool depthPeeling)
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(300, 300);

  if (depthPeeling)
  {
    renWin->SetMultiSamples(1);
    renWin->SetAlphaBitPlanes(1);
    renderer->SetUseDepthPeeling(1);
    renderer->SetMaximumNumberOfPeels(200);
    renderer->SetOcclusionRatio(0.1);
  }

  renderer->AddActor(textActor);

  textProperty->SetJustificationToCentered();
  textProperty->SetVerticalJustificationToCentered(); // default
  textProperty->SetFontFamilyToArial();               // default.
  textProperty->SetFontSize(36);

  renWin->Render();
  renderer->ResetCamera();

  renWin->Render();
  if (depthPeeling && !renderer->GetLastRenderingUsedDepthPeeling())
  {
    std::cerr << "depth peeling was not used\n";
  }

  // usual font issues so we up the tolerance a bit
  // int retVal = vtkTesting::Test(argc, argv, renWin, 0.07);
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
}
