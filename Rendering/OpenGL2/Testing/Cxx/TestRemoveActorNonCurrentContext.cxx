/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRemoveActorNonCurrentContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Test for releasing graphics resources from a non-current
// render window with vtkPolyDataMapper

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

//-----------------------------------------------------------------------------
class TestRemoveActorNonCurrentContextCallback : public vtkCommand
{
public:
  static TestRemoveActorNonCurrentContextCallback* New()
  {
    return new TestRemoveActorNonCurrentContextCallback;
  }

  void Execute(vtkObject* caller, unsigned long eventId, void* vtkNotUsed(callData)) override
  {
    if (eventId != vtkCommand::KeyPressEvent)
    {
      return;
    }

    vtkRenderWindowInteractor* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    if (interactor == nullptr)
    {
      return;
    }

    char* pressedKey = interactor->GetKeySym();

    if (strcmp(pressedKey, "9") == 0)
    {
      renderer2->RemoveAllViewProps();
      renderWindow1->Render();
      renderWindow2->Render();
    }
  }

  vtkRenderer* renderer1;
  vtkRenderer* renderer2;
  vtkRenderWindow* renderWindow1;
  vtkRenderWindow* renderWindow2;
};

//-----------------------------------------------------------------------------
int TestRemoveActorNonCurrentContext(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper);

  vtkNew<vtkRenderer> renderer1;
  vtkNew<vtkRenderWindow> renderWindow1;
  vtkNew<vtkRenderWindowInteractor> interactor1;

  renderWindow1->SetParentId(nullptr);
  renderWindow1->AddRenderer(renderer1);
  renderWindow1->SetWindowName("Victim");
  renderWindow1->SetSize(500, 300);
  renderWindow1->SetPosition(100, 100);
  interactor1->SetRenderWindow(renderWindow1);

  renderer1->AddActor(sphereActor);
  renderer1->SetBackground(1.0, 1.0, 1.0);

  // Create the second renderwindow/renderer/mapper.
  // This is the renderer we later remove all the actors from,
  // triggering the problems in the first renderer
  vtkNew<vtkRenderer> renderer2;
  vtkNew<vtkRenderWindow> renderWindow2;
  vtkNew<vtkRenderWindowInteractor> interactor2;

  renderWindow2->SetParentId(nullptr);
  renderWindow2->AddRenderer(renderer2);
  renderWindow2->SetWindowName("Villain");
  renderWindow2->SetSize(300, 300);
  renderWindow2->SetPosition(650, 100);
  interactor2->SetRenderWindow(renderWindow2);

  renderer2->AddActor(coneActor);
  renderer2->SetBackground(1.0, 1.0, 1.0);

  // Create callback so we can trigger the problem
  vtkNew<TestRemoveActorNonCurrentContextCallback> callback;
  callback->renderer1 = renderer1;
  callback->renderer2 = renderer2;
  callback->renderWindow1 = renderWindow1;
  callback->renderWindow2 = renderWindow2;
  interactor1->AddObserver("KeyPressEvent", callback);

  // Let's go
  interactor1->Initialize();
  renderWindow1->Render();
  renderWindow2->Render();
  renderWindow1->MakeCurrent();
  interactor1->SetKeyEventInformation(0, 0, 0, 0, "9");
  interactor1->InvokeEvent(vtkCommand::KeyPressEvent, nullptr);
  int retval = vtkTesting::Test(argc, argv, renderWindow1, 10);
  if (retval == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor1->Start();
  }
  return !retval;
}
