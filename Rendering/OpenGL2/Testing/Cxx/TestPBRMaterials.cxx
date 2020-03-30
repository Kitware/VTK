/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBRMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the PBR Interpolation shading
// It renders spheres with different materials using a skybox as image based lighting

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLSkybox.h"
#include "vtkOpenGLTexture.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

//----------------------------------------------------------------------------
int TestPBRMaterials(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkOpenGLSkybox> skybox;

  vtkSmartPointer<vtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  vtkSmartPointer<vtkPBRPrefilterTexture> prefilter = renderer->GetEnvMapPrefiltered();
  prefilter->SetPrefilterSamples(64);
  prefilter->SetPrefilterSize(64);

  vtkNew<vtkOpenGLTexture> textureCubemap;
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

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(100);

  vtkNew<vtkPolyDataMapper> pdSphere;
  pdSphere->SetInputConnection(sphere->GetOutputPort());

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 0.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 1.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.72, 0.45, 0.2);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(1.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 2.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 3.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(0.0, 1.0, 1.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  for (int i = 0; i < 6; i++)
  {
    vtkNew<vtkActor> actorSphere;
    actorSphere->SetPosition(i, 4.0, 0.0);
    actorSphere->SetMapper(pdSphere);
    actorSphere->GetProperty()->SetInterpolationToPBR();
    actorSphere->GetProperty()->SetColor(1.0, 0.0, 0.0);
    actorSphere->GetProperty()->SetOpacity(1.0);
    actorSphere->GetProperty()->SetMetallic(0.0);
    actorSphere->GetProperty()->SetRoughness(i / 5.0);
    renderer->AddActor(actorSphere);
  }

  skybox->SetTexture(textureCubemap);
  renderer->AddActor(skybox);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
