// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the PBR Anisotropy feature
// It renders spheres with different anisotropy values

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkOpenGLTexture.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTextureMapToSphere.h"

//----------------------------------------------------------------------------
int TestPBRAnisotropy(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  renderer->UseSphericalHarmonicsOff();

  vtkNew<vtkOpenGLTexture> textureCubemap;
  textureCubemap->CubeMapOn();
  textureCubemap->UseSRGBColorSpaceOn();

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

  renderer->SetEnvironmentTexture(textureCubemap);
  renderer->UseImageBasedLightingOn();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(75);
  sphere->SetPhiResolution(75);

  vtkNew<vtkTextureMapToSphere> textureMap;
  textureMap->SetInputConnection(sphere->GetOutputPort());
  textureMap->PreventSeamOff();

  vtkNew<vtkPolyDataTangents> tangents;
  tangents->SetInputConnection(textureMap->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->RotateX(20);
    actorSphere->RotateY(20);
    actorSphere->SetMapper(mapper);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetAnisotropy(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 1.0, 0.0);
    actorSphere->RotateX(20);
    actorSphere->RotateY(20);
    actorSphere->SetMapper(mapper);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(0.1);
    actorSphere->GetProperty()->SetAnisotropy(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 2.0, 0.0);
    actorSphere->RotateX(20);
    actorSphere->RotateY(20);
    actorSphere->SetMapper(mapper);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(0.1);
    actorSphere->GetProperty()->SetAnisotropy(1.0);
    actorSphere->GetProperty()->SetAnisotropyRotation(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
