/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLightingMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test ... TO DO
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkActor.h"
#include "vtkCameraPass.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkLight.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkLightingMapPass.h"


int TestLightingMapNormalsPass(int argc, char *argv[])
{
  bool interactive = false;

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-I"))
    {
      interactive = true;
    }
  }

  // 0. Prep data
  const char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkSmartPointer<vtkPLYReader> reader
    = vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  vtkSmartPointer<vtkPolyDataMapper> mapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  //actor->GetProperty()->SetRepresentationToWireframe();

  // 1. Set up renderer, window, & interactor
  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  vtkSmartPointer<vtkRenderWindow> window =
    vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::New();

  window->AddRenderer(renderer);
  interactor->SetRenderWindow(window);

  vtkSmartPointer<vtkLight> light =
    vtkSmartPointer<vtkLight>::New();
  light->SetLightTypeToSceneLight();
  light->SetPosition(0.0, 0.0, 1.0);
  light->SetPositional(true);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetIntensity(1.0);

  renderer->AddLight(light);

  renderer->AddActor(actor);

  // 2. Set up rendering passes
  vtkSmartPointer<vtkLightingMapPass> lightingPass =
    vtkSmartPointer<vtkLightingMapPass>::New();
  lightingPass->SetRenderType(vtkLightingMapPass::NORMALS);

  vtkSmartPointer<vtkRenderPassCollection> passes =
    vtkSmartPointer<vtkRenderPassCollection>::New();
  passes->AddItem(lightingPass);

  vtkSmartPointer<vtkSequencePass> sequence =
    vtkSmartPointer<vtkSequencePass>::New();
  sequence->SetPasses(passes);

  vtkSmartPointer<vtkCameraPass> cameraPass =
    vtkSmartPointer<vtkCameraPass>::New();
  cameraPass->SetDelegatePass(sequence);

  vtkOpenGLRenderer *glRenderer =
    vtkOpenGLRenderer::SafeDownCast(renderer.GetPointer());
  glRenderer->SetPass(cameraPass);

  // 3. Render image and compare against baseline
  window->Render();

  int retVal = vtkRegressionTestImage(window);
  if (interactive || retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !retVal;
}
