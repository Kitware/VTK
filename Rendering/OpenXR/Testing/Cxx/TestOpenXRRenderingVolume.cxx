// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkNew.h"
#include "vtkOpenXRCamera.h"
#include "vtkOpenXRRenderWindow.h"
#include "vtkOpenXRRenderWindowInteractor.h"
#include "vtkOpenXRRenderer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//------------------------------------------------------------------------------
// Render a opaque wavelet using volume rendering
// ensuring the generated image looks the same
// whatever the viewpoint
int TestOpenXRRenderingVolume(int argc, char* argv[])
{
  vtkNew<vtkOpenXRRenderer> renderer;
  vtkNew<vtkOpenXRRenderWindow> renderWindow;
  vtkNew<vtkOpenXRCamera> cam;
  vtkNew<vtkOpenXRRenderWindowInteractor> iren;
  vtkNew<vtkActor> actor;

  renderer->SetBackground(0.2, 0.3, 0.4);
  renderer->SetActiveCamera(cam);
  renderer->AddActor(actor);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);
  iren->SetActionManifestDirectory("../../");

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.5);
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.8, 0.4);
  ctf->AddRGBPoint(157.091, 0.2, 0.8, 0.4);
  ctf->AddRGBPoint(276.829, 0.2, 0.8, 0.4);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 1.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);
  volumeProperty->SetShade(0);
  volumeProperty->SetScalarOpacityUnitDistance(1.732);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  renderer->AddVolume(volume);

  iren->Initialize();
  iren->DoOneEvent(renderWindow, renderer);
  iren->DoOneEvent(renderWindow, renderer); // Needed by monado so that it starts to render

  renderWindow->Render();
  int retVal = vtkRegressionTester::Test(argc, argv, renderWindow, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return 0;
}
