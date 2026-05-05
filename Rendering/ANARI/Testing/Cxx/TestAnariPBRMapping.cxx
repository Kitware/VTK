// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * This test covers the PBR Interpolation shading. It renders a cube with custom texture mapping
 *
 * This test requires the ANARI_KHR_MATERIAL_PHYSICALLY_BASED ANARI extension. If this extension is
 * not available (with helide backend for example), it behaves as a smoke test to make sure the VTK
 * API does not crash. If the loaded backend supports the extension, it will perform an image
 * comparison.
 */

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkImageFlip.h"
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
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

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
  light->SetIntensity(0.8);

  renderer->AddLight(light);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

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
  material->Update();

  vtkNew<vtkPNGReader> albedoReader;
  char* colname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Base_Color.png");
  albedoReader->SetFileName(colname);
  delete[] colname;

  vtkNew<vtkTexture> albedo;
  albedo->UseSRGBColorSpaceOn();
  albedo->InterpolateOn();
  albedo->MipmapOn();
  albedo->SetInputConnection(albedoReader->GetOutputPort());
  albedo->Update();

  vtkNew<vtkPNGReader> normalReader;
  char* normname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Normal.png");
  normalReader->SetFileName(normname);
  delete[] normname;

  vtkNew<vtkTexture> normal;
  normal->InterpolateOn();
  normal->MipmapOn();
  normal->SetInputConnection(normalReader->GetOutputPort());
  normal->Update();

  vtkNew<vtkPNGReader> anisotropyReader;
  char* anisotropyname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vtk_Anisotropy.png");
  anisotropyReader->SetFileName(anisotropyname);
  delete[] anisotropyname;

  vtkNew<vtkTexture> anisotropy;
  anisotropy->InterpolateOn();
  anisotropy->MipmapOn();
  anisotropy->SetInputConnection(anisotropyReader->GetOutputPort());
  anisotropy->Update();

  vtkNew<vtkActor> actor;
  actor->SetOrientation(0.0, 25.0, 0.0);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  // ORM texture value is scaled by the Occlusion strength,
  // roughness coefficient and metallic coefficient.
  // Set to 1.0 as they act as multipliers with texture value
  actor->GetProperty()->SetOcclusionStrength(1.0);
  actor->GetProperty()->SetMetallic(1.0);
  actor->GetProperty()->SetRoughness(1.0);

  // The anisotropy texture contains two independent components corresponding
  // to the anisotropy value and anisotropy rotation.
  //
  // The anisotropy texture value is scaled by the anisotropy coefficient
  // of the material. The anisotropy rotation rotates the direction of the
  // anisotropy (ie. the tangent) around the normal and is not scaled by the
  // anisotropy rotation coefficient.
  actor->GetProperty()->SetAnisotropy(1.0);
  actor->GetProperty()->SetAnisotropyRotation(1.0);

  actor->GetProperty()->SetBaseColorTexture(albedo);
  actor->GetProperty()->SetORMTexture(material);
  actor->GetProperty()->SetNormalTexture(normal);
  actor->GetProperty()->SetAnisotropyTexture(anisotropy);
  renderer->AddActor(actor);

  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);
  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariPBRMapping");

  renWin->Render();
  renderer->GetActiveCamera()->Zoom(1.5);
  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  bool testSuccess = true;
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

    testSuccess = retVal == vtkRegressionTester::PASSED;
  }

  return testSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
