/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayRenderMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can render at resolutions larger than the window
// by rendering and stitching multiple tiles.

#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkWindowToImageFilter.h"

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayTiling(int argc, char* argv[])
{
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);
  vtkOSPRayRendererNode::SetBackgroundMode(2, renderer);

  vtkSmartPointer<vtkLight> l = vtkSmartPointer<vtkLight>::New();
  l->SetLightTypeToHeadlight();
  l->SetIntensity(0.1);
  renderer->AddLight(l);

  // todo: as soon as we get materials, make the bunny reflective
  // to really show off
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkSmartPointer<vtkPLYReader> polysource = vtkSmartPointer<vtkPLYReader>::New();
  polysource->SetFileName(fileName);

  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  renWin->SetSize(400, 400);

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

  vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
  vtkSmartPointer<vtkJPEGReader> imgReader = vtkSmartPointer<vtkJPEGReader>::New();
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;
  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(textr);

  double up[3] = { 0.0, 1.0, 0.0 };
  double east[3] = { -1.0, 0.0, 0.0 };
  vtkOSPRayRendererNode::SetNorthPole(up, renderer);
  vtkOSPRayRendererNode::SetEastPole(east, renderer);

  renWin->Render();

  vtkSmartPointer<vtkWindowToImageFilter> w2i = vtkSmartPointer<vtkWindowToImageFilter>::New();
  w2i->SetInput(renWin);
  w2i->SetScale(4, 4);
  w2i->Update();

  // vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
  // writer->SetFileName("screenshot.png");
  // writer->SetInputConnection(w2i->GetOutputPort());
  // writer->Write();

  // Show stitched image in separate window
  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(w2i->GetOutput());
  vtkNew<vtkRenderer> ren2;
  ren2->AddActor(imageActor);

  // Background color white to distinguish image boundary
  ren2->SetEnvironmentalBG(1, 1, 1);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(ren2);
  renderWindow->Render();

  vtkSmartPointer<vtkOSPRayTestInteractor> style = vtkSmartPointer<vtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
