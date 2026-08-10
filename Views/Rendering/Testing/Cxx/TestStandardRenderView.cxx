// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>

#include "vtkConeSource.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkSphereSource.h"
#include "vtkStandardRenderView.h"
#include "vtkSurfaceRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestStandardRenderView(int argc, char* argv[])
{
  // Create a view
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);

  // Test background methods
  view->SetBackground(0.1, 0.2, 0.3);
  view->SetBackground2(0.0, 0.0, 0.1);
  view->SetGradientBackground(true);

  // Add two representations
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(-1.0, 0.0, 0.0);
  sphere->SetThetaResolution(32);
  sphere->SetPhiResolution(32);

  vtkNew<vtkSurfaceRepresentation> sphereRep;
  sphereRep->SetInputConnection(sphere->GetOutputPort());
  sphereRep->SetColor(0.8, 0.2, 0.2);
  view->AddRepresentation(sphereRep);

  vtkNew<vtkConeSource> cone;
  cone->SetCenter(1.0, 0.0, 0.0);
  cone->SetResolution(32);

  vtkNew<vtkSurfaceRepresentation> coneRep;
  coneRep->SetInputConnection(cone->GetOutputPort());
  coneRep->SetColor(0.2, 0.2, 0.8);
  coneRep->SetRepresentationToSurfaceWithEdges();
  view->AddRepresentation(coneRep);

  // Test orientation axes
  if (!view->GetOrientationAxesVisibility())
  {
    std::cerr << "Orientation axes should be visible by default." << std::endl;
    return EXIT_FAILURE;
  }
  if (view->GetOrientationAxesInteractive())
  {
    std::cerr << "Orientation axes should be non-interactive by default." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetOrientationAxesInteractive(true);
  if (!view->GetOrientationAxesInteractive())
  {
    std::cerr << "SetOrientationAxesInteractive failed." << std::endl;
    return EXIT_FAILURE;
  }
  view->SetOrientationAxesInteractive(false);

  view->SetOrientationAxesVisibility(false);
  if (view->GetOrientationAxesVisibility())
  {
    std::cerr << "SetOrientationAxesVisibility(false) failed." << std::endl;
    return EXIT_FAILURE;
  }
  view->SetOrientationAxesVisibility(true);

  if (!view->GetOrientationMarkerWidget())
  {
    std::cerr << "GetOrientationMarkerWidget() returned null." << std::endl;
    return EXIT_FAILURE;
  }

  // Test lighting - light kit
  if (view->GetUseLightKit())
  {
    std::cerr << "Light kit should be off by default." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetUseLightKit(true);
  if (!view->GetUseLightKit())
  {
    std::cerr << "SetUseLightKit(true) failed." << std::endl;
    return EXIT_FAILURE;
  }

  // Test light kit parameter forwarding
  view->SetKeyLightIntensity(1.0);
  if (view->GetKeyLightIntensity() != 1.0)
  {
    std::cerr << "SetKeyLightIntensity failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetKeyToFillRatio(5.0);
  if (view->GetKeyToFillRatio() != 5.0)
  {
    std::cerr << "SetKeyToFillRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetKeyToHeadRatio(4.0);
  if (view->GetKeyToHeadRatio() != 4.0)
  {
    std::cerr << "SetKeyToHeadRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetKeyToBackRatio(4.5);
  if (view->GetKeyToBackRatio() != 4.5)
  {
    std::cerr << "SetKeyToBackRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetKeyLightWarmth(0.7);
  if (view->GetKeyLightWarmth() != 0.7)
  {
    std::cerr << "SetKeyLightWarmth failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetFillLightWarmth(0.3);
  view->SetHeadLightWarmth(0.5);
  view->SetBackLightWarmth(0.4);
  view->SetKeyLightAngle(45.0, -20.0);
  view->SetFillLightAngle(-70.0, -10.0);
  view->SetBackLightAngle(0.0, 110.0);

  view->SetMaintainLuminance(true);
  if (!view->GetMaintainLuminance())
  {
    std::cerr << "SetMaintainLuminance failed." << std::endl;
    return EXIT_FAILURE;
  }
  view->SetMaintainLuminance(false);

  if (!view->GetLightKit())
  {
    std::cerr << "GetLightKit() returned null." << std::endl;
    return EXIT_FAILURE;
  }

  view->SetUseLightKit(false);

  // Test individual light management
  vtkNew<vtkLight> light;
  light->SetPosition(10.0, 10.0, 10.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetColor(1.0, 1.0, 0.8);
  light->SetIntensity(0.5);
  view->AddLight(light);

  view->RemoveLight(light);
  view->RemoveAllLights();

  // Re-enable light kit for final render
  view->SetUseLightKit(true);

  // Test removal
  if (view->GetNumberOfRepresentations() != 2)
  {
    std::cerr << "Expected 2 representations, got " << view->GetNumberOfRepresentations()
              << std::endl;
    return EXIT_FAILURE;
  }

  // Render
  view->ResetCamera();
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->Start();
  }
  return !retVal;
}
