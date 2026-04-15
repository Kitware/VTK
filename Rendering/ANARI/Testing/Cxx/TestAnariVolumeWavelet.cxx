// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

#include <iostream>

static const char* TestAnariVolumeWaveletLog = "# StreamVersion 1\n"
                                               "EnterEvent 299 0 0 0 0 0 0\n"
                                               "MouseMoveEvent 218 272 0 0 0 0 0\n"
                                               "LeftButtonPressEvent 218 272 0 0 0 0 0\n"
                                               "StartInteractionEvent 218 272 0 0 0 0 0\n"
                                               "MouseMoveEvent 250 181 0 0 0 0 0\n"
                                               "RenderEvent 250 181 0 0 0 0 0\n"
                                               "InteractionEvent 250 181 0 0 0 0 0\n"
                                               "MiddleButtonReleaseEvent 250 181 0 0 0 0 0\n"
                                               "EndInteractionEvent 250 181 0 0 0 0 0\n"
                                               "RenderEvent 250 181 0 0 0 0 0\n"
                                               "MouseMoveEvent 384 1 0 0 0 0 0\n"
                                               "LeaveEvent 399 -8 0 0 0 0 0\n";

int TestAnariVolumeWavelet(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkTest::ErrorObserver> errorObserver;

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.5);
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());
  volumeMapper->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);
  volumeProperty->SetShade(0);
  volumeProperty->SetScalarOpacityUnitDistance(1.732);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Create the renderwindow, interactor and renderer
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(401, 399); // NPOT size
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.3, 0.4);
  renderWindow->AddRenderer(renderer);

  renderer->AddVolume(volume);

  // Attach ANARI render pass
  vtkNew<vtkAnariPass> anariPass;
  renderer->SetPass(anariPass);

  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariVolumeWavelet");

  renderer->ResetCamera();
  renderWindow->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    iren->Initialize();

    vtkNew<vtkInteractorEventRecorder> recorder;
    recorder->SetInteractor(iren);
    recorder->ReadFromInputStringOn();
    recorder->SetInputString(TestAnariVolumeWaveletLog);
    recorder->Play();
    recorder->Off();

    int retVal = vtkRegressionTestImage(renderWindow);
    return !retVal;
  }

  std::cout << "Required feature KHR_VOLUME_TRANSFER_FUNCTION1D not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
