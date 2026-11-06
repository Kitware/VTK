// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the attenuation of the base layer by the PBR clear coat
// It renders spheres whose coat reflects an increasing share of the environment, so the base
// layer underneath has to darken by the same share. The coat index of refraction is swept well
// past the default, because a coat that only reflects a few percent moves too few pixels to
// hold the backends to each other.
// The baseline is shared by every rendering backend, which keeps them at parity: the WebGPU
// backend renders it within 4e-4 of the OpenGL backend. It is a test of appearance and of that
// parity. TestPBRWhiteFurnace is the one that checks that the layering conserves energy.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkImageFlip.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

//------------------------------------------------------------------------------
int TestPBRCoatAttenuation(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  // a dim light keeps the direct lobe in the picture without drowning out the
  // image based lighting the attenuation applies to
  vtkNew<vtkLight> light;
  light->SetPosition(2.0, 1.0, 2.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);
  light->SetIntensity(0.1);
  renderer->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(660, 220);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkTexture> textureCubemap;
  textureCubemap->CubeMapOn();

  std::string pathSkybox[6] = { "Data/skybox/posx.jpg", "Data/skybox/negx.jpg",
    "Data/skybox/posy.jpg", "Data/skybox/negy.jpg", "Data/skybox/posz.jpg",
    "Data/skybox/negz.jpg" };

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkJPEGReader> jpg;
    char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, pathSkybox[i].c_str());
    jpg->SetFileName(fname);
    delete[] fname;
    vtkNew<vtkImageFlip> flip;
    flip->SetInputConnection(jpg->GetOutputPort());
    flip->SetFilteredAxis(1); // flip y axis
    textureCubemap->SetInputConnection(i, flip->GetOutputPort());
  }

  renderer->SetEnvironmentTexture(textureCubemap, true);
  renderer->UseImageBasedLightingOn();
  renderer->UseSphericalHarmonicsOff();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  // a white dielectric under a coat of growing strength: its diffuse lobe is attenuated twice
  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.9, 0.9, 0.9);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(0.4);
    actorSphere->GetProperty()->SetCoatIOR(3.0);
    actorSphere->GetProperty()->SetCoatRoughness(0.05);
    actorSphere->GetProperty()->SetCoatStrength(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  // a rough metal under a coat of growing index of refraction: its specular lobe is attenuated
  // once, and the sharp reflection of the coat stands out against the blurred one of the base
  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 1.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.72, 0.45, 0.2);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(0.7);
    actorSphere->GetProperty()->SetCoatIOR(1.0 + (i * 0.6));
    actorSphere->GetProperty()->SetCoatRoughness(0.05);
    actorSphere->GetProperty()->SetCoatStrength(1.0);
    renderer->AddActor(actorSphere);
  }

  // frame the two rows tightly, and in parallel projection so that every sphere reflects the
  // environment from the same direction and only the material tells them apart
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(1.1);
  camera->SetPosition(2.5, 0.5, 5.0);
  camera->SetFocalPoint(2.5, 0.5, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
