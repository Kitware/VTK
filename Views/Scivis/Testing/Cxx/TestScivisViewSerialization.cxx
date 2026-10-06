// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Views/Scivis opts into VTK_MARSHALAUTO code generation. None of the regular
// image-regression tests in this directory exercise it: the automatic SerDes
// variant of an image-regression test round-trips the render window, which
// never references a vtkScivisView or a vtkScivisRepresentation directly. This
// test instead registers a view with a vtkObjectManager, transfers its state to
// a second manager, and checks that what was configured on the original is
// still there on the object rebuilt from that state.

#include "vtkColorTransferFunction.h"
#include "vtkLightKit.h"
#include "vtkNew.h"
#include "vtkObjectManager.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScivisView.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceRepresentation.h"
#include "vtkVolumeRepresentation.h"

#include <cmath>
#include <iostream>

namespace
{
bool DoublesEqual(double a, double b, double tol = 1e-9)
{
  return std::abs(a - b) <= tol;
}
}

int TestScivisViewSerialization(int, char*[])
{
  vtkNew<vtkScivisView> view;
  view->SetBackground(0.1, 0.2, 0.3);
  view->SetBackground2(0.4, 0.5, 0.6);
  view->SetWindowSize(400, 300);
  view->SetOrientationAxesVisibility(false);
  view->SetUseLightKit(true);
  view->GetLightKit()->SetKeyLightIntensity(0.75);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkSurfaceRepresentation> surfaceRep;
  surfaceRep->SetInputConnection(sphere->GetOutputPort());
  surfaceRep->SetColor(0.8, 0.2, 0.1);
  surfaceRep->SetOpacity(0.5);
  surfaceRep->SetRepresentationToSurfaceWithEdges();
  view->AddRepresentation(surfaceRep);

  vtkNew<vtkRTAnalyticSource> volumeSource;
  vtkNew<vtkVolumeRepresentation> volumeRep;
  volumeRep->SetInputConnection(volumeSource->GetOutputPort());
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(255.0, 1.0, 0.0, 0.0);
  volumeRep->SetColorTransferFunction(ctf);
  view->AddRepresentation(volumeRep);

  vtkNew<vtkObjectManager> serManager;
  if (!serManager->Initialize())
  {
    std::cerr << "Failed to initialize the serializing object manager." << std::endl;
    return EXIT_FAILURE;
  }

  // With no display available the view's render window carries a
  // vtkTestingInteractor, which is not serializable. Detach it for the round
  // trip, as vtkTesting::SerDesTest does, and restore it afterwards. The
  // orientation marker widget holds its own reference to the interactor, so
  // it needs the same treatment. The render window's reference is counted,
  // so detaching would destroy the interactor; the smart pointers keep it
  // alive until it is restored.
  vtkSmartPointer<vtkRenderWindow> rw = view->GetRenderWindow();
  vtkSmartPointer<vtkRenderWindowInteractor> interactor = rw->GetInteractor();
  vtkOrientationMarkerWidget* markerWidget = view->GetOrientationMarkerWidget();
  vtkSmartPointer<vtkRenderWindowInteractor> widgetInteractor = markerWidget->GetInteractor();
  if (interactor && interactor->IsA("vtkTestingInteractor"))
  {
    rw->SetInteractor(nullptr);
  }
  if (widgetInteractor && widgetInteractor->IsA("vtkTestingInteractor"))
  {
    markerWidget->SetInteractor(nullptr);
  }

  const vtkTypeUInt32 viewId = serManager->RegisterObject(view);
  serManager->UpdateStatesFromObjects();

  vtkNew<vtkObjectManager> deserManager;
  if (!deserManager->Initialize())
  {
    std::cerr << "Failed to initialize the deserializing object manager." << std::endl;
    return EXIT_FAILURE;
  }
  const std::vector<vtkTypeUInt32> ids = serManager->GetAllDependencies(vtkObjectManager::ROOT());
  for (const auto& id : ids)
  {
    deserManager->RegisterState(serManager->GetState(id));
  }
  for (const auto& hash : serManager->GetBlobHashes(ids))
  {
    deserManager->RegisterBlob(hash, serManager->GetBlob(hash));
  }
  deserManager->UpdateObjectsFromStates();

  auto* newView = vtkScivisView::SafeDownCast(deserManager->GetObjectAtId(viewId));
  if (!newView)
  {
    std::cerr << "Failed to deserialize vtkScivisView." << std::endl;
    return EXIT_FAILURE;
  }
  if (newView == view.Get())
  {
    std::cerr << "Deserialization did not produce a new object." << std::endl;
    return EXIT_FAILURE;
  }

  int retVal = EXIT_SUCCESS;
  auto check = [&retVal](bool condition, const char* what)
  {
    if (!condition)
    {
      std::cerr << "Mismatch after round trip: " << what << std::endl;
      retVal = EXIT_FAILURE;
    }
  };

  double rgb[3];
  newView->GetBackground(rgb);
  check(DoublesEqual(rgb[0], 0.1) && DoublesEqual(rgb[1], 0.2) && DoublesEqual(rgb[2], 0.3),
    "view background");
  newView->GetBackground2(rgb);
  check(DoublesEqual(rgb[0], 0.4) && DoublesEqual(rgb[1], 0.5) && DoublesEqual(rgb[2], 0.6),
    "view background2");

  int size[2];
  newView->GetWindowSize(size);
  check(size[0] == 400 && size[1] == 300, "view window size");
  check(!newView->GetOrientationAxesVisibility(), "orientation axes visibility");
  check(newView->GetUseLightKit(), "use light kit");
  check(newView->GetLightKit() != nullptr, "light kit object");
  if (newView->GetLightKit())
  {
    check(
      DoublesEqual(newView->GetLightKit()->GetKeyLightIntensity(), 0.75), "light kit intensity");
  }
  check(newView->GetNumberOfRepresentations() == 2, "number of representations");

  auto* newSurfaceRep = vtkSurfaceRepresentation::SafeDownCast(newView->GetRepresentation(0));
  check(newSurfaceRep != nullptr, "surface representation type");
  if (newSurfaceRep)
  {
    newSurfaceRep->GetColor(rgb);
    check(DoublesEqual(rgb[0], 0.8) && DoublesEqual(rgb[1], 0.2) && DoublesEqual(rgb[2], 0.1),
      "surface representation color");
    check(DoublesEqual(newSurfaceRep->GetOpacity(), 0.5), "surface representation opacity");
    check(newSurfaceRep->GetRepresentation() == vtkSurfaceRepresentation::SURFACE_WITH_EDGES,
      "surface representation representation type");
  }

  auto* newVolumeRep = vtkVolumeRepresentation::SafeDownCast(newView->GetRepresentation(1));
  check(newVolumeRep != nullptr, "volume representation type");
  if (newVolumeRep)
  {
    auto* newCtf = newVolumeRep->GetColorTransferFunction();
    check(newCtf != nullptr, "volume representation color transfer function");
    if (newCtf)
    {
      check(newCtf->GetSize() == 2, "color transfer function point count");
    }
  }

  markerWidget->SetInteractor(widgetInteractor);
  rw->SetInteractor(interactor);

  return retVal;
}
