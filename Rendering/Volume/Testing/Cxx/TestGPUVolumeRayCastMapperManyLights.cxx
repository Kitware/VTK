/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPUVolumeRayCastMapperManyLights.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPUVolumeRayCastMapperManyLights(int, char*[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

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
  ctf->AddRGBPoint(37.3531, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(276.829, 0.0, 0.0, 0.0);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);
  volumeProperty->SetShade(1);
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
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  int nr_lights = 8;

  renderer->RemoveAllLights();

  for (int i = 0; i < nr_lights; i++)
  {
    vtkNew<vtkLight> light;
    // everything so the volume isn't lighted
    light->SetPositional(true);
    light->SetPosition(-15.0, 15.0, 0.0);
    light->SetFocalPoint(-30.0, 15.0, 0.0);
    light->SetConeAngle(1.0);
    light->SetIntensity(1.0);
    renderer->AddLight(light);
  }

  renderer->AddVolume(volume);
  renderer->ResetCamera();
  renderWindow->Render();

  return 0;
}
