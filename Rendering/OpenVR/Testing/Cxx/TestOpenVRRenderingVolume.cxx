// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkNew.h"
#include "vtkOpenVRCamera.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//------------------------------------------------------------------------------
// Render a opaque wavelet using volume rendering
// ensuring the generated image looks the same
// whatever the viewpoint
int TestOpenVRRenderingVolume(int argc, char* argv[])
{
  vtkNew<vtkOpenVRRenderer> renderer;
  vtkNew<vtkOpenVRRenderWindow> renderWindow;
  vtkNew<vtkOpenVRCamera> cam;
  vtkNew<vtkOpenVRRenderWindowInteractor> iren;
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

  renderWindow->Initialize();
  if (!renderWindow->GetHMD())
  {
    return 1;
  }

  iren->Initialize();
  iren->DoOneEvent(renderWindow, renderer);

  renderWindow->Render();
  int retVal = vtkRegressionTester::Test(argc, argv, renderWindow, 10);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return 0;
}
