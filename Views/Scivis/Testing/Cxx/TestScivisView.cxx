// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>

#include "vtkConeSource.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestScivisView(int argc, char* argv[])
{
  // Create a view
  vtkNew<vtkScivisView> view;
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
  if ((view->GetOrientationMarkerWidget()->GetInteractive() != 0))
  {
    std::cerr << "Orientation axes should be non-interactive by default." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetOrientationMarkerWidget()->SetInteractive(true);
  if (!(view->GetOrientationMarkerWidget()->GetInteractive() != 0))
  {
    std::cerr << "SetOrientationAxesInteractive failed." << std::endl;
    return EXIT_FAILURE;
  }
  view->GetOrientationMarkerWidget()->SetInteractive(false);

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
  view->GetLightKit()->SetKeyLightIntensity(1.0);
  if (view->GetLightKit()->GetKeyLightIntensity() != 1.0)
  {
    std::cerr << "SetKeyLightIntensity failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetLightKit()->SetKeyToFillRatio(5.0);
  if (view->GetLightKit()->GetKeyToFillRatio() != 5.0)
  {
    std::cerr << "SetKeyToFillRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetLightKit()->SetKeyToHeadRatio(4.0);
  if (view->GetLightKit()->GetKeyToHeadRatio() != 4.0)
  {
    std::cerr << "SetKeyToHeadRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetLightKit()->SetKeyToBackRatio(4.5);
  if (view->GetLightKit()->GetKeyToBackRatio() != 4.5)
  {
    std::cerr << "SetKeyToBackRatio failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetLightKit()->SetKeyLightWarmth(0.7);
  if (view->GetLightKit()->GetKeyLightWarmth() != 0.7)
  {
    std::cerr << "SetKeyLightWarmth failed." << std::endl;
    return EXIT_FAILURE;
  }

  view->GetLightKit()->SetFillLightWarmth(0.3);
  view->GetLightKit()->SetHeadLightWarmth(0.5);
  view->GetLightKit()->SetBackLightWarmth(0.4);
  view->GetLightKit()->SetKeyLightAngle(45.0, -20.0);
  view->GetLightKit()->SetFillLightAngle(-70.0, -10.0);
  view->GetLightKit()->SetBackLightAngle(0.0, 110.0);

  view->GetLightKit()->SetMaintainLuminance(true);
  if (!view->GetLightKit()->GetMaintainLuminance())
  {
    std::cerr << "SetMaintainLuminance failed." << std::endl;
    return EXIT_FAILURE;
  }
  view->GetLightKit()->SetMaintainLuminance(false);

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
  view->GetRenderer()->AddLight(light);

  view->GetRenderer()->RemoveLight(light);
  view->GetRenderer()->RemoveAllLights();

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
