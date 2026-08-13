// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <iostream>

int TestGPUVolumeRayCastMapper(int argc, char* argv[])
{
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      std::cerr << "GL" << std::endl;
      useOSP = false;
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
  renderer->ResetCamera();
  renderWindow->Render();

  // Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP)
  {
    renderer->SetPass(osprayPass);
  }

  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetPosition(-1.169110, 19.253188, -2.291590);
  renderer->GetActiveCamera()->SetFocalPoint(-0.810639, 5.734724, -10.041192);
  renderer->GetActiveCamera()->SetViewUp(0.036873, 0.497734, -0.866546);

  volumeMapper->DebugOn();

  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);

  int retVal;
  if (valid)
  {
    retVal = vtkRegressionTestImage(renderWindow);
  }
  else
  {
    retVal = vtkTesting::NOT_RUN;
    std::cout << "Required extensions not supported." << std::endl;
  }

  return !retVal;
}
