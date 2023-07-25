// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkMultiVolume.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

//------------------------------------------------------------------------------
int TestGPURayCastMultiVolumeClipping(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> source1;
  source1->SetWholeExtent(-100, 100, -100, 0, -100, 100);
  vtkNew<vtkRTAnalyticSource> source2;
  source2->SetWholeExtent(-100, 100, 0, 100, -100, 100);

  vtkNew<vtkColorTransferFunction> colorTransferFunction1;
  colorTransferFunction1->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction1->AddRGBPoint(140.0, 0.0, 1.0, 1.0);
  colorTransferFunction1->AddRGBPoint(80.0, 1.0, 1.0, 0.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity1;
  scalarOpacity1->AddPoint(220.0, 1.0);
  scalarOpacity1->AddPoint(190.0, 0.6);
  scalarOpacity1->AddPoint(150.0, 0.2);

  vtkNew<vtkPiecewiseFunction> GradientOpacity1;
  GradientOpacity1->AddPoint(0.0, 0.0);
  GradientOpacity1->AddPoint(25.0, 1.0);

  vtkNew<vtkColorTransferFunction> colorTransferFunction2;
  colorTransferFunction2->AddRGBPoint(220.0, 0.0, 1.0, 0.0);
  colorTransferFunction2->AddRGBPoint(140.0, 0.0, 1.0, 1.0);
  colorTransferFunction2->AddRGBPoint(80.0, 1.0, 0.0, 1.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity2;
  scalarOpacity2->AddPoint(220.0, 1.0);
  scalarOpacity2->AddPoint(150.0, 0.2);
  scalarOpacity2->AddPoint(190.0, 0.6);

  vtkNew<vtkPiecewiseFunction> gradientOpacity2;
  gradientOpacity2->AddPoint(0.0, 0.0);
  gradientOpacity2->AddPoint(25.0, 1.0);
  gradientOpacity2->AddPoint(50.0, 0.0);

  vtkNew<vtkVolume> volume1;
  volume1->GetProperty()->SetInterpolationTypeToLinear();
  volume1->GetProperty()->SetColor(colorTransferFunction1);
  volume1->GetProperty()->SetScalarOpacity(scalarOpacity1);
  volume1->GetProperty()->SetGradientOpacity(GradientOpacity1);
  volume1->GetProperty()->ShadeOn();
  volume1->RotateX(-75.);

  vtkNew<vtkVolume> volume2;
  volume2->GetProperty()->SetInterpolationTypeToLinear();
  volume2->GetProperty()->SetColor(colorTransferFunction2);
  volume2->GetProperty()->SetScalarOpacity(scalarOpacity2);
  volume2->GetProperty()->SetGradientOpacity(gradientOpacity2);
  volume2->GetProperty()->ShadeOn();

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkMultiVolume> overlappingVolumes;
  overlappingVolumes->SetMapper(volumeMapper);

  volumeMapper->SetInputConnection(0, source1->GetOutputPort());
  overlappingVolumes->SetVolume(volume1, 0);

  volumeMapper->SetInputConnection(2, source2->GetOutputPort());
  overlappingVolumes->SetVolume(volume2, 2);

  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0, 1, 0);
  volumeMapper->AddClippingPlane(plane);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(overlappingVolumes);
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->GetActiveCamera()->SetPosition(-1000, 0, 0);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }
  return !retVal;
}
