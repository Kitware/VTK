// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test volume renders a synthetic dataset with four different
// positional lights in the scene.

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <vtkActor.h>
#include <vtkContourFilter.h>
#include <vtkLightActor.h>
#include <vtkPolyDataMapper.h>

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"

int TestAnariPositionalLights(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  reader->Update();
  delete[] volumeFile;

  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.1);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.0, 0.0, 0.4);
  ren->AutomaticLightCreationOff();
  ren->RemoveAllLights();

  vtkNew<vtkLight> light1;
  light1->SetLightTypeToSceneLight();
  light1->SetPositional(true);
  light1->SetDiffuseColor(1, 0, 0);
  light1->SetAmbientColor(0, 0, 0);
  light1->SetSpecularColor(1, 1, 1);
  light1->SetConeAngle(60);
  light1->SetPosition(0.0, 0.0, 100.0);
  light1->SetFocalPoint(0.0, 0.0, 0.0);
  ren->AddLight(light1);

  vtkNew<vtkLightActor> lightActor;
  lightActor->SetLight(light1);
  ren->AddViewProp(lightActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* const colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();

  double scalarRange[2];
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volume->SetPosition(-30.0, 0.0, 0.0);

  ren->AddViewProp(volume);

  vtkNew<vtkContourFilter> cf;
  cf->SetValue(0, 60.0);
  cf->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkPolyDataMapper> pm;
  pm->SetInputConnection(cf->GetOutputPort());
  pm->SetScalarVisibility(0);

  vtkNew<vtkActor> ac;
  ac->SetMapper(pm);
  ac->SetPosition(-89.0, 0.0, 0.0);
  ren->AddActor(ac);

  vtkNew<vtkActor> ac1;
  ac1->SetMapper(pm);
  ac1->SetPosition(0, 0, 0);
  ren->SetTwoSidedLighting(0);

  // Attach ANARI render pass
  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, ren);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariPositionalLights";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), ren);
  }

  vtkAnariRendererNode::SetLibraryName("environment", ren);
  vtkAnariRendererNode::SetSamplesPerPixel(5, ren);
  vtkAnariRendererNode::SetLightFalloff(.5, ren);
  vtkAnariRendererNode::SetUseDenoiser(1, ren);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren);

  renWin->Render();
  ren->ResetCamera();
  iren->Initialize();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_LIGHT_SPOT)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_LIGHT_SPOT not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
