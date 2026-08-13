// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test renders a volume slab and a plane through the middle of the slab.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <iostream>

//------------------------------------------------------------------------------
int TestGPURayCastVolumePlane(int argc, char* argv[])
{
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-49, 50, -49, 50, 0, 1);
  source->SetCenter(0.0, 0.0, 0.0);
  source->Update();

  vtkImageData* wavelet = source->GetOutput();
  wavelet->SetSpacing(1, 1, 10);

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->SetSampleDistance(1.0);
  volumeMapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.8, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 1.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 301);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkPlaneSource> plane;
  plane->SetCenter(0, 0, 5.0);
  plane->SetOrigin(-70, -70, 5.0);
  plane->SetPoint1(70, -70, 5.0);
  plane->SetPoint2(-70, 70, 5.0);

  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper.GetPointer());
  planeActor->GetProperty()->SetAmbient(1);
  planeActor->GetProperty()->SetDiffuse(0);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.35, 0.45);
  renderWindow->AddRenderer(renderer.GetPointer());

  renderer->AddActor(planeActor.GetPointer());
  renderer->AddVolume(volume.GetPointer());

  vtkNew<vtkCamera> camera;
  camera->SetViewUp(-0.914424, 0.097608, -0.392813);
  camera->SetFocalPoint(0.000000, 0.000000, 5.000000);
  camera->SetPosition(10.020837, 93.043621, 4.792618);
  renderer->SetActiveCamera(camera);

  int ret = vtkRegressionTestImage(renderWindow);
  return ret == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
