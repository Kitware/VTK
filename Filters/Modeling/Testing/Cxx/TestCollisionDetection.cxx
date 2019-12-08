/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestButterflyScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCollisionDetectionFilter.h"
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNamedColors.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTextActor.h"
#include "vtkTransform.h"
#include <chrono>
#include <sstream>
#include <string>
#include <thread>

int TestCollisionDetection(int argc, char* argv[])
{
  int contactMode = 0;
  if (argc > 1)
  {
    contactMode = std::stoi(std::string(argv[1]));
  }
  vtkSmartPointer<vtkSphereSource> sphere0 = vtkSmartPointer<vtkSphereSource>::New();
  sphere0->SetRadius(.29);
  sphere0->SetPhiResolution(31);
  sphere0->SetThetaResolution(31);
  sphere0->SetCenter(0.0, 0, 0);

  vtkSmartPointer<vtkSphereSource> sphere1 = vtkSmartPointer<vtkSphereSource>::New();
  sphere1->SetPhiResolution(30);
  sphere1->SetThetaResolution(30);
  sphere1->SetRadius(0.3);

  vtkSmartPointer<vtkMatrix4x4> matrix1 = vtkSmartPointer<vtkMatrix4x4>::New();
  vtkSmartPointer<vtkTransform> transform0 = vtkSmartPointer<vtkTransform>::New();

  vtkSmartPointer<vtkCollisionDetectionFilter> collide =
    vtkSmartPointer<vtkCollisionDetectionFilter>::New();
  collide->SetInputConnection(0, sphere0->GetOutputPort());
  collide->SetTransform(0, transform0);
  collide->SetInputConnection(1, sphere1->GetOutputPort());
  collide->SetMatrix(1, matrix1);
  collide->SetBoxTolerance(0.0);
  collide->SetCellTolerance(0.0);
  collide->SetNumberOfCellsPerNode(2);
  if (contactMode == 0)
  {
    collide->SetCollisionModeToAllContacts();
  }
  else if (contactMode == 1)
  {
    collide->SetCollisionModeToFirstContact();
  }
  else
  {
    collide->SetCollisionModeToHalfContacts();
  }
  collide->GenerateScalarsOn();

  // Visualize
  vtkSmartPointer<vtkNamedColors> colors = vtkSmartPointer<vtkNamedColors>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper1 = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper1->SetInputConnection(collide->GetOutputPort(0));
  mapper1->ScalarVisibilityOff();
  vtkSmartPointer<vtkActor> actor1 = vtkSmartPointer<vtkActor>::New();
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->BackfaceCullingOn();
  actor1->SetUserTransform(transform0);
  actor1->GetProperty()->SetDiffuseColor(colors->GetColor3d("tomato").GetData());
  actor1->GetProperty()->SetRepresentationToWireframe();

  vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper2->SetInputConnection(collide->GetOutputPort(1));

  vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->BackfaceCullingOn();
  actor2->SetUserMatrix(matrix1);

  vtkSmartPointer<vtkPolyDataMapper> mapper3 = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper3->SetInputConnection(collide->GetContactsOutputPort());
  mapper3->SetResolveCoincidentTopologyToPolygonOffset();

  vtkSmartPointer<vtkActor> actor3 = vtkSmartPointer<vtkActor>::New();
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(0, 0, 0);
  actor3->GetProperty()->SetLineWidth(3.0);

  vtkSmartPointer<vtkTextActor> txt = vtkSmartPointer<vtkTextActor>::New();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->UseHiddenLineRemovalOn();
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(txt);
  renderer->SetBackground(0.5, 0.5, 0.5);

  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(640, 480);
  renderWindow->AddRenderer(renderer);

  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow(renderWindow);

  // Move the first object
  double dx = .1;
  int numSteps = 20;
  transform0->Translate(-numSteps * dx - .3, 0.0, 0.0);
  renderWindow->Render();
  renderer->GetActiveCamera()->Azimuth(-45);
  renderer->GetActiveCamera()->Elevation(45);
  renderer->GetActiveCamera()->Dolly(1.2);

  for (int i = 0; i < numSteps; ++i)
  {
    transform0->Translate(dx, 0.0, 0.0);
    renderer->ResetCameraClippingRange();
    std::ostringstream textStream;
    textStream << collide->GetCollisionModeAsString() << ": Number of contact cells is "
               << collide->GetNumberOfContacts();
    txt->SetInput(textStream.str().c_str());
    renderWindow->Render();
    if (collide->GetNumberOfContacts() > 0)
    {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  renderer->ResetCamera();
  renderWindow->Render();
  interactor->Start();
  collide->Print(std::cout);
  return EXIT_SUCCESS;
}
