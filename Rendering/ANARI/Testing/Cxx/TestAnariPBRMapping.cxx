// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the PBR Interpolation shading
// It renders a cube with custom texture mapping

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkJPEGReader.h"
#include "vtkLight.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataTangents.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

//------------------------------------------------------------------------------
int TestAnariPBRMapping(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = true;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderer> renderer;
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

  vtkNew<vtkTexture> textureCubemap;
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
  material->MipmapOn();
  material->SetInputConnection(materialReader->GetOutputPort());

  vtkNew<vtkPNGReader> albedoReader;
  char* colname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Base_Color.png");
  albedoReader->SetFileName(colname);
  delete[] colname;

  vtkNew<vtkTexture> albedo;
  albedo->UseSRGBColorSpaceOn();
  albedo->InterpolateOn();
  albedo->MipmapOn();
  albedo->SetInputConnection(albedoReader->GetOutputPort());

  vtkNew<vtkPNGReader> normalReader;
  char* normname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Normal.png");
  normalReader->SetFileName(normname);
  delete[] normname;

  vtkNew<vtkTexture> normal;
  normal->InterpolateOn();
  normal->MipmapOn();
  normal->SetInputConnection(normalReader->GetOutputPort());

  vtkNew<vtkPNGReader> anisotropyReader;
  char* anisotropyname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Anisotropy.png");
  anisotropyReader->SetFileName(anisotropyname);
  delete[] anisotropyname;

  vtkNew<vtkTexture> anisotropy;
  anisotropy->InterpolateOn();
  anisotropy->MipmapOn();
  anisotropy->SetInputConnection(anisotropyReader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetOrientation(0.0, 25.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  // set metallic, roughness, anisotropy and anisotropyRotation
  // to 1.0 as they act as multipliers with texture value
  actor->GetProperty()->SetMetallic(1.0);
  actor->GetProperty()->SetRoughness(1.0);
  actor->GetProperty()->SetAnisotropy(1.0);
  actor->GetProperty()->SetAnisotropyRotation(1.0);

  actor->GetProperty()->SetBaseColorTexture(albedo);
  actor->GetProperty()->SetORMTexture(material);
  actor->GetProperty()->SetNormalTexture(normal);
  actor->GetProperty()->SetAnisotropyTexture(anisotropy);

  renderer->AddActor(actor);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, renderer);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariPBRMapping";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), renderer);
  }

  vtkAnariRendererNode::SetLibraryName("environment", renderer);
  vtkAnariRendererNode::SetSamplesPerPixel(6, renderer);
  vtkAnariRendererNode::SetLightFalloff(.5, renderer);
  vtkAnariRendererNode::SetUseDenoiser(1, renderer);
  vtkAnariRendererNode::SetCompositeOnGL(1, renderer);

  renWin->Render();
  renderer->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_MATERIAL_PHYSICALLY_BASED)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      vtkNew<vtkAnariTestInteractor> style;
      style->SetPipelineControlPoints(renderer, anariPass, nullptr);
      iren->SetInteractorStyle(style);
      style->SetCurrentRenderer(renderer);

      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_MATERIAL_PHYSICALLY_BASED not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
