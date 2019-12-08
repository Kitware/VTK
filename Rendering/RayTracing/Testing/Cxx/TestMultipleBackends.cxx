/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultipleBackends.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can use the different raytracing backends alongside each other
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

#include "vtkOSPRayTestInteractor.h"

int TestMultipleBackends(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Define viewport ranges
  double xmins[4] = { 0, .5, 0, .5 };
  double xmaxs[4] = { 0.5, 1, 0.5, 1 };
  double ymins[4] = { 0, 0, .5, .5 };
  double ymaxs[4] = { 0.5, 0.5, 1, 1 };
  for (unsigned i = 0; i < 4; i++)
  {
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

    renderWindow->AddRenderer(renderer);
    renderer->SetViewport(xmins[i], ymins[i], xmaxs[i], ymaxs[i]);

    renderer->SetBackground(0.75, 0.75, 0.75);

    if (i == 1)
    {
      // VisRTX
      vtkSmartPointer<vtkOSPRayPass> visrtxpass = vtkSmartPointer<vtkOSPRayPass>::New();
      renderer->SetPass(visrtxpass);
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
    }
    else if (i == 2)
    {
      // OSPRay
      vtkSmartPointer<vtkOSPRayPass> ospraypass = vtkSmartPointer<vtkOSPRayPass>::New();
      renderer->SetPass(ospraypass);
    }
    else if (i == 3)
    {
      // OSPRay Path Tracer
      vtkSmartPointer<vtkOSPRayPass> ospraypathtracerpass = vtkSmartPointer<vtkOSPRayPass>::New();
      renderer->SetPass(ospraypathtracerpass);
      vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
    }

    // Create a sphere
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->SetCenter(0.0, 0.0, 0.0);
    sphereSource->SetPhiResolution(10);
    sphereSource->SetRadius(5);
    sphereSource->Update();

    // Create a mapper and actor
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphereSource->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    renderer->AddActor(actor);
    renderer->ResetCamera();

    renderWindow->Render();
    renderWindow->SetWindowName("Multiple ViewPorts");
  }

  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  renderWindowInteractor->SetInteractorStyle(style);

  renderWindowInteractor->Start();

  return 0;
}
