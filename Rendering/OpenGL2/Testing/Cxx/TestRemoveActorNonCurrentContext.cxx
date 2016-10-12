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
class TestRemoveActorNonCurrentContextCallback: public vtkCommand
{
public:

  static TestRemoveActorNonCurrentContextCallback *New()
  {
    return new TestRemoveActorNonCurrentContextCallback;
  }

  void Execute(vtkObject* caller,
                       unsigned long eventId,
                       void* vtkNotUsed(callData)) VTK_OVERRIDE
  {
    if (eventId != vtkCommand::KeyPressEvent)
    {
      return;
    }

    vtkRenderWindowInteractor* interactor =
      static_cast<vtkRenderWindowInteractor*>(caller);
    if (interactor == NULL)
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
  sphereActor->SetMapper(sphereMapper.GetPointer());

  vtkNew<vtkConeSource> cone;
  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper.GetPointer());

  vtkNew<vtkRenderer> renderer1;
  vtkNew<vtkRenderWindow> renderWindow1;
  vtkNew<vtkRenderWindowInteractor> interactor1;

  renderWindow1->SetParentId(0);
  renderWindow1->AddRenderer(renderer1.GetPointer());
  renderWindow1->SetWindowName("Victim");
  renderWindow1->SetSize(500, 300);
  renderWindow1->SetPosition(100, 100);
  interactor1->SetRenderWindow(renderWindow1.GetPointer());

  renderer1->AddActor(sphereActor.GetPointer());
  renderer1->SetBackground(1.0, 1.0, 1.0);

  // Create the second renderwindow/renderer/mapper.
  // This is the renderer we later remove all the actors from,
  // triggering the problems in the first renderer
  vtkNew<vtkRenderer> renderer2;
  vtkNew<vtkRenderWindow> renderWindow2;
  vtkNew<vtkRenderWindowInteractor> interactor2;

  renderWindow2->SetParentId(0);
  renderWindow2->AddRenderer(renderer2.GetPointer());
  renderWindow2->SetWindowName("Villain");
  renderWindow2->SetSize(300, 300);
  renderWindow2->SetPosition(650, 100);
  interactor2->SetRenderWindow(renderWindow2.GetPointer());

  renderer2->AddActor(coneActor.GetPointer());
  renderer2->SetBackground(1.0, 1.0, 1.0);

  // Create callback so we can trigger the problem
  vtkNew<TestRemoveActorNonCurrentContextCallback> callback;
  callback->renderer1 = renderer1.GetPointer();
  callback->renderer2	= renderer2.GetPointer();
  callback->renderWindow1 = renderWindow1.GetPointer();
  callback->renderWindow2 = renderWindow2.GetPointer();
  interactor1->AddObserver("KeyPressEvent", callback.GetPointer());

  // Let's go
  interactor1->Initialize();
  renderWindow1->Render();
  renderWindow2->Render();
  renderWindow1->MakeCurrent();
  interactor1->SetKeyEventInformation(0, 0, 0, 0, "9");
  interactor1->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
  int retval = vtkTesting::Test(argc, argv,
                                renderWindow1.GetPointer(), 10);
  if (retval == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor1->Start();
  }
  return !retval;
}
