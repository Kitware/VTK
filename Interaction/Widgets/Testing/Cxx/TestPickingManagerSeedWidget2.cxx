// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause AND Apache-2.0
//
// This example tests the PickingManager using a scene full of seed widgets.
// It makes sure that the picking works when some widgets are disabled.
//
// The test depends on:
// * vtkSeedWidget
// * vtkSphereHandleRepresentation
//
// By default the Picking Manager is enabled.
// Press 'Alt' to enable/disable some of seeds.
// Press 'Space' to restore the cube

// VTK includes
#include "vtkCommand.h"
#include "vtkHandleWidget.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPickingManager.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkSmartPointer.h"
#include "vtkSphereHandleRepresentation.h"
#include "vtkTimerLog.h"

// STL includes
#include <fstream>
#include <iostream>
#include <list>

const char eventLogTestPickingManagerSeedWidget2[] =
  ""
  "# StreamVersion 1\n"
  "EnterEvent 599 295 0 0 0 0 0\n"
  "MouseMoveEvent 599 295 0 0 0 0 0\n"
  "MouseMoveEvent 419 243 0 0 0 0 0\n"
  "MouseMoveEvent 417 243 0 0 0 0 0\n"
  "LeftButtonPressEvent 417 243 0 0 0 0 0\n"
  "StartInteractionEvent 417 243 0 0 0 0 0\n"
  "MouseMoveEvent 414 243 0 0 0 0 0\n"
  "MouseMoveEvent 412 243 0 0 0 0 0\n"
  "MouseMoveEvent 294 228 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 294 228 0 0 0 0 0\n"
  "KeyPressEvent 297 223 0 0 0 1 Alt_L\n"
  "RenderEvent 297 225 0 0 0 0 Alt_L\n"
  "KeyReleaseEvent 299 225 0 0 0 1 Alt_L\n"
  "LeftButtonPressEvent 324 237 0 0 0 0 Alt_L\n"
  "RenderEvent 324 237 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 237 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 235 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 233 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 349 113 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 347 113 0 0 0 0 Alt_L\n"
  "LeftButtonReleaseEvent 347 113 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 347 113 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 347 115 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 347 118 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 344 120 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 322 323 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 322 325 0 0 0 0 Alt_L\n"
  "LeftButtonPressEvent 322 325 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 325 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 328 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 324 330 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 314 423 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 314 425 0 0 0 0 Alt_L\n"
  "LeftButtonReleaseEvent 314 425 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 314 425 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 314 423 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 317 420 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 554 568 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 564 580 0 0 0 0 Alt_L\n"
  "MouseMoveEvent 574 595 0 0 0 0 Alt_L\n"
  "ExitEvent 574 595 0 0 0 0 Alt_L\n";

//------------------------------------------------------------------------------
// Press 'Space' to reorganize the cube of seeds
class vtkPickingManagerSeedWidgetTest2Callback : public vtkCommand
{
public:
  static vtkPickingManagerSeedWidgetTest2Callback* New()
  {
    return new vtkPickingManagerSeedWidgetTest2Callback;
  }

  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkRenderWindowInteractor* iren = static_cast<vtkRenderWindowInteractor*>(caller);
    char* cKeySym = iren->GetKeySym();
    std::string keySym = cKeySym != nullptr ? cKeySym : "";

    // Reorganize the cube
    if (keySym == "space")
    {
      const int baseCube = static_cast<int>(pow(this->Seeds.size(), 1. / 3.) / 2 + 0.5);
      std::list<vtkSmartPointer<vtkHandleWidget>>::iterator it = this->Seeds.begin();

      for (int i = -baseCube; i < baseCube; ++i)
      {
        for (int j = -baseCube; j < baseCube; ++j)
        {
          for (int k = -baseCube; k < baseCube; ++k)
          {
            vtkSphereHandleRepresentation* newHandleRep =
              vtkSphereHandleRepresentation::SafeDownCast((*it)->GetRepresentation());

            double pos[3] = { static_cast<double>(i), static_cast<double>(j),
              static_cast<double>(k) };
            newHandleRep->SetWorldPosition(pos);

            ++it;
          }
        }
      }
    }
    // Disable every other seed
    if (keySym == "Alt_L" || keySym == "Alt_R")
    {
      const int baseCube = static_cast<int>(pow(this->Seeds.size(), 1. / 3.) / 2 + 0.5);
      int n = 0;
      for (int i = -baseCube; i < baseCube; ++i)
      {
        for (int j = -baseCube; j < baseCube; ++j)
        {
          for (int k = -baseCube; k < baseCube; ++k)
          {
            if (n % 2 == 0)
            {
              vtkHandleWidget* w = this->Widget->GetSeed(n);
              w->SetEnabled(!w->GetEnabled());
            }
            ++n;
          }
        }
      }
      this->Widget->GetCurrentRenderer()->Render();
    }
  }

  std::list<vtkSmartPointer<vtkHandleWidget>> Seeds;
  vtkSeedWidget* Widget;
};

//------------------------------------------------------------------------------
// Test Picking Manager with a lot of seeds
//------------------------------------------------------------------------------
int TestPickingManagerSeedWidget2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> interactor;
  vtkNew<vtkInteractorStyleTrackballCamera> interactorStyle;
  interactor->SetRenderWindow(renderWindow);
  interactor->SetInteractorStyle(interactorStyle);

  /*--------------------------------------------------------------------------*/
  // PICKING MANAGER
  /*--------------------------------------------------------------------------*/
  interactor->GetPickingManager()->EnabledOn();

  /*--------------------------------------------------------------------------*/
  // SEEDS
  /*--------------------------------------------------------------------------*/
  // Representations
  double pos[3] = { 0, 0, 0 };
  vtkNew<vtkSphereHandleRepresentation> handle;
  // handle->SetHandleSize(15.0);
  handle->GetProperty()->SetRepresentationToWireframe();
  handle->GetProperty()->SetColor(1, 1, 1);

  vtkNew<vtkSeedRepresentation> seedRepresentation;
  seedRepresentation->SetHandleRepresentation(handle);

  // Settings
  vtkNew<vtkSeedWidget> seedWidget;
  seedWidget->SetRepresentation(seedRepresentation);
  seedWidget->SetInteractor(interactor);
  seedWidget->EnabledOn();

  // Create a cube full of seeds
  // base correspond to the side of the cube --> (2*base)^3 seeds
  const int baseCube = 2;
  std::list<vtkSmartPointer<vtkHandleWidget>> seeds;
  for (int i = -baseCube; i < baseCube; ++i)
  {
    for (int j = -baseCube; j < baseCube; ++j)
    {
      for (int k = -baseCube; k < baseCube; ++k)
      {
        vtkHandleWidget* newHandle = seedWidget->CreateNewHandle();
        newHandle->SetEnabled(1);
        vtkSphereHandleRepresentation* newHandleRep =
          vtkSphereHandleRepresentation::SafeDownCast(newHandle->GetRepresentation());

        pos[0] = i;
        pos[1] = j;
        pos[2] = k;
        newHandleRep->GetProperty()->SetRepresentationToWireframe();
        newHandleRep->GetProperty()->SetColor(1, 1, 1);
        newHandleRep->SetWorldPosition(pos);

        seeds.emplace_back(newHandle);
      }
    }
  }
  seedWidget->CompleteInteraction();

  // Callback to reorganize the cube when space is pressed
  vtkNew<vtkPickingManagerSeedWidgetTest2Callback> callback;
  callback->Seeds = seeds;
  callback->Widget = seedWidget;
  interactor->AddObserver(vtkCommand::KeyPressEvent, callback);

  /*--------------------------------------------------------------------------*/
  // Rendering
  /*--------------------------------------------------------------------------*/
  // Add the actors to the renderer, set the background and size
  renderer->SetBackground(0.1, 0.2, 0.4);
  renderWindow->SetSize(600, 600);

  // Record
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(interactor);
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLogTestPickingManagerSeedWidget2);

  // render the image
  interactor->Initialize();
  double extent[6] = { -7, 7, -7, 7, -1, 1 };
  renderer->ResetCamera(extent);
  renderWindow->Render();

  recorder->Play();
  recorder->Off();

  interactor->Start();

  return EXIT_SUCCESS;
}
