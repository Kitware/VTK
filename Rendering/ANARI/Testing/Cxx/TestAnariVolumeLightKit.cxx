// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers using VTK light kit to add general purpose lighting
// in a simple, flexible, and attractive way.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkLightKit.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"

int TestAnariVolumeLightKit(int argc, char* argv[])
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

  double scalarRange[2];

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  reader->Update();
  delete[] volumeFile;

  volumeMapper->SetInputConnection(reader->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(1);
  volumeMapper->SetSampleDistance(0.01);

  vtkNew<vtkLightKit> lightKit;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.0, 0.0, 0.0);
  ren->SetTwoSidedLighting(0);

  renWin->AddRenderer(ren);

  lightKit->SetKeyLightWarmth(1.0);
  lightKit->SetFillLightWarmth(0.0);
  lightKit->SetBackLightWarmth(0.0);
  lightKit->AddLightsToRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(55, 0.0);
  scalarOpacity->AddPoint(65, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.0);
  volumeProperty->SetDiffuse(1.0);
  volumeProperty->SetSpecular(0.0);
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* const colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);

  // Attach ANARI render pass
  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, ren);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariVolumeLightKit";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), ren);
  }

  vtkAnariRendererNode::SetLibraryName("environment", ren);
  vtkAnariRendererNode::SetSamplesPerPixel(6, ren);
  vtkAnariRendererNode::SetLightFalloff(.5, ren);
  vtkAnariRendererNode::SetUseDenoiser(1, ren);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren);

  renWin->Render();
  ren->ResetCamera();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Initialize();
      iren->SetDesiredUpdateRate(30.0);
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_VOLUME_TRANSFER_FUNCTION1D not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
