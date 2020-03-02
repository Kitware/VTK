/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBRMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the PBR Interpolation shading
// It renders a cube with custom texture mapping

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
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
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"

//----------------------------------------------------------------------------
int TestPBRMapping(int argc, char* argv[])
{
  vtkNew<vtkOpenGLRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  vtkNew<vtkLight> light;
  light->SetPosition(2.0, 0.0, 2.0);
  light->SetFocalPoint(0.0, 0.0, 0.0);

  renderer->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkPBRIrradianceTexture> irradiance = renderer->GetEnvMapIrradiance();
  irradiance->SetIrradianceStep(0.3);
  vtkSmartPointer<vtkPBRPrefilterTexture> prefilter = renderer->GetEnvMapPrefiltered();
  prefilter->SetPrefilterSamples(64);
  prefilter->SetPrefilterSize(64);

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

  vtkNew<vtkCubeSource> cube;

  vtkNew<vtkTriangleFilter> triangulation;
  triangulation->SetInputConnection(cube->GetOutputPort());

  vtkNew<vtkPolyDataTangents> tangents;
  tangents->SetInputConnection(triangulation->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(tangents->GetOutputPort());

  vtkNew<vtkPNGReader> materialReader;
  char* matname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Material.png");
  materialReader->SetFileName(matname);
  delete[] matname;

  vtkNew<vtkTexture> material;
  material->InterpolateOn();
  material->SetInputConnection(materialReader->GetOutputPort());

  vtkNew<vtkPNGReader> albedoReader;
  char* colname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Base_Color.png");
  albedoReader->SetFileName(colname);
  delete[] colname;

  vtkNew<vtkTexture> albedo;
  albedo->UseSRGBColorSpaceOn();
  albedo->InterpolateOn();
  albedo->SetInputConnection(albedoReader->GetOutputPort());

  vtkNew<vtkPNGReader> normalReader;
  char* normname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Normal.png");
  normalReader->SetFileName(normname);
  delete[] normname;

  vtkNew<vtkTexture> normal;
  normal->InterpolateOn();
  normal->SetInputConnection(normalReader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetOrientation(0.0, 25.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  // set metallic and roughness to 1.0 as they act as multipliers with texture value
  actor->GetProperty()->SetMetallic(1.0);
  actor->GetProperty()->SetRoughness(1.0);

  actor->GetProperty()->SetBaseColorTexture(albedo);
  actor->GetProperty()->SetORMTexture(material);
  actor->GetProperty()->SetNormalTexture(normal);

  renderer->AddActor(actor);

  renWin->Render();

  renderer->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
