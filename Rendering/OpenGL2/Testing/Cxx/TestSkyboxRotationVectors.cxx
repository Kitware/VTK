// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHDRReader.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSkybox.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

int TestSkyboxRotationVectors(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLSkybox> skybox;

  vtkNew<vtkHDRReader> reader;
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");
  reader->SetFileName(fname);
  delete[] fname;
  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->MipmapOn();
  texture->InterpolateOn();
  texture->SetInputConnection(reader->GetOutputPort());

  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);

  renderer->SetEnvironmentUp(0, 0, 1);
  renderer->SetEnvironmentRight(1, 0, 0);

  skybox->SetProjection(vtkSkybox::Sphere);
  skybox->SetTexture(texture);

  renderer->AddActor(skybox);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actorSphere;
  actorSphere->SetMapper(pdSphere);
  actorSphere->GetProperty()->SetInterpolationToPBR();
  actorSphere->GetProperty()->SetMetallic(1.0);
  actorSphere->GetProperty()->SetRoughness(0.3);
  renderer->AddActor(actorSphere);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
