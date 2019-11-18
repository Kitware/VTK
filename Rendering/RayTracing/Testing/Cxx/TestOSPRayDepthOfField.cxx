/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayDepthOfField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that the perspective camera's focal distance and
// aperture size work correctly.

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkOSPRayCameraNode.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayDepthOfField(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);
  renWin->SetSize(400, 400);

  vtkSmartPointer<vtkLight> l = vtkSmartPointer<vtkLight>::New();
  l->SetLightTypeToHeadlight();
  l->SetIntensity(1.0);
  renderer->AddLight(l);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkSmartPointer<vtkPLYReader> polysource = vtkSmartPointer<vtkPLYReader>::New();
  polysource->SetFileName(fileName);

  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());

  vtkSmartPointer<vtkActor> actor1 = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor1);
  actor1->SetMapper(mapper);
  actor1->SetPosition(0, -0.05, 0);

  vtkSmartPointer<vtkActor> actor2 = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor2);
  actor2->SetMapper(mapper);
  actor2->SetPosition(0, -0.05, 0.3);

  vtkSmartPointer<vtkActor> actor3 = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor3);
  actor3->SetMapper(mapper);
  actor3->SetPosition(0, -0.05, -0.3);

  vtkSmartPointer<vtkOSPRayPass> ospray = vtkSmartPointer<vtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  vtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(-0.3f, 0.2f, 1.0f);

  // Init focal distance
  camera->SetFocalDistance(camera->GetDistance());

  // Increase focal disk
  for (int i = 9; i < 100; i += 10)
  {
    camera->SetFocalDisk(i * 0.01f);
    renWin->Render();
  }

  // Decrease focal disk
  for (int i = 9; i < 100; i += 10)
  {
    camera->SetFocalDisk(1.0f - 0.8f * (i * 0.01f));
    renWin->Render();
  }

  // Move focal point
  for (int i = 9; i < 200; i += 10)
  {
    camera->SetFocalDistance(camera->GetDistance() + sin(i * 0.03141592653) * 0.3);
    renWin->Render();
  }

  iren->Start();
  return 0;
}
