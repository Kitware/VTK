// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <iostream>

static const char* TestGPUVolumeRayCastMapperLog = "# StreamVersion 1\n"
                                                   "EnterEvent 299 0 0 0 0 0 0\n"
                                                   "LeftButtonPressEvent 218 272 0 0 0 0 0\n"
                                                   "MouseMoveEvent 219 273 0 0 0 0 0\n"
                                                   "MouseMoveEvent 223 152 0 0 0 0 0\n"
                                                   "LeftButtonReleaseEvent 223 152 0 0 0 0 0\n"
                                                   "MouseMoveEvent 222 151 0 0 0 0 0\n"
                                                   "MouseMoveEvent 216 200 0 0 0 0 0\n"
                                                   "RightButtonPressEvent 216 200 0 0 0 0 0\n"
                                                   "MouseMoveEvent 216 203 0 0 0 0 0\n"
                                                   "MouseMoveEvent 233 352 0 0 0 0 0\n"
                                                   "RightButtonReleaseEvent 233 352 0 0 0 0 0\n"
                                                   "MouseMoveEvent 232 353 0 0 0 0 0\n"
                                                   "MiddleButtonPressEvent 232 353 0 0 0 0 0\n"
                                                   "MouseMoveEvent 232 352 0 0 0 0 0\n"
                                                   "MouseMoveEvent 259 125 0 0 0 0 0\n"
                                                   "MiddleButtonReleaseEvent 259 125 0 0 0 0 0\n"
                                                   "MouseMoveEvent 259 124 0 0 0 0 0\n"
                                                   "MouseMoveEvent 220 80 0 0 0 0 0\n"
                                                   "RightButtonPressEvent 220 80 0 0 0 0 0\n"
                                                   "MouseMoveEvent 220 82 0 0 0 0 0\n"
                                                   "MouseMoveEvent 225 233 0 0 0 0 0\n"
                                                   "RightButtonReleaseEvent 225 233 0 0 0 0 0\n"
                                                   "MouseMoveEvent 227 235 0 0 0 0 0\n"
                                                   "MouseMoveEvent 247 258 0 0 0 0 0\n"
                                                   "MiddleButtonPressEvent 247 258 0 0 0 0 0\n"
                                                   "MouseMoveEvent 247 256 0 0 0 0 0\n"
                                                   "MouseMoveEvent 250 181 0 0 0 0 0\n"
                                                   "MiddleButtonReleaseEvent 250 181 0 0 0 0 0\n"
                                                   "LeaveEvent 399 -8 0 0 0 0 0\n";

int TestGPUVolumeRayCastMapper(int argc, char* argv[])
{
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;

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
  renderer->ResetCamera();
  renderWindow->Render();

  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);

  int retVal;
  if (valid)
  {
    retVal = !(vtkTesting::InteractorEventLoop(argc, argv, iren, TestGPUVolumeRayCastMapperLog));
  }
  else
  {
    retVal = vtkTesting::PASSED;
    std::cout << "Required extensions not supported." << std::endl;
  }

  return !retVal;
}
