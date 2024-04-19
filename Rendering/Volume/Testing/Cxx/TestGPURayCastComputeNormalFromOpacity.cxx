// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastComputeNormalFromOpacity(int, char*[])
{
  // we generate a volumetric sphere
  double origin[3] = { 0, 0, 0 };
  double spacing[3] = { 0.005, 0.005, 0.005 };
  int dimension[3] = { 200, 200, 200 };

  vtkNew<vtkImageData> sphere;
  sphere->SetOrigin(origin);
  sphere->SetSpacing(spacing);
  sphere->SetDimensions(dimension);

  // the volume has scalar 1 outside the sphere and scalar 0 inside

  vtkNew<vtkFloatArray> dataArray;
  dataArray->SetNumberOfComponents(1);
  dataArray->SetNumberOfTuples(dimension[0] * dimension[1] * dimension[2]);

  for (int i = 0; i < dimension[0]; i++)
  {
    for (int j = 0; j < dimension[1]; j++)
    {
      for (int k = 0; k < dimension[2]; k++)
      {
        int coords2[3];
        coords2[0] = i - dimension[0] / 2;
        coords2[1] = j - dimension[1] / 2;
        coords2[2] = k - dimension[2] / 2;
        int dist2 = coords2[0] * coords2[0] + coords2[1] * coords2[1] + coords2[2] * coords2[2];
        int array_idx = k * dimension[0] * dimension[1] + j * dimension[0] + i;
        float val = (dist2 > 0.20 * dimension[0] * dimension[0]) ? 1.0 : 0.0;
        dataArray->SetValue(array_idx, val);
      }
    }
  }

  sphere->GetPointData()->SetScalars(dataArray);

  vtkNew<vtkTest::ErrorObserver> errorObserver;

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(spacing[0]);
  volumeMapper->SetInputData(sphere);
  volumeMapper->SetComputeNormalFromOpacity(true);
  volumeMapper->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkNew<vtkVolumeProperty> volumeProperty;

  // opacity TF : decreasing, so that the sphere can appear
  vtkNew<vtkPiecewiseFunction> compositeOpacity;
  compositeOpacity->AddPoint(0.0, 1.0);
  compositeOpacity->AddPoint(1.0, 0.0);
  volumeProperty->SetScalarOpacity(compositeOpacity);

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 0.196, 0.659, 0.337);
  ctf->AddRGBPoint(1.0, 0.196, 0.659, 0.337);
  volumeProperty->SetColor(ctf);

  volumeProperty->SetDiffuse(1.0);
  volumeProperty->SetSpecular(1.0);
  volumeProperty->SetShade(1);
  volumeProperty->SetScalarOpacityUnitDistance(spacing[0]);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Create the renderwindow, interactor and renderer
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(401, 399);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.4, 0.4, 0.4);
  renderer->SetTwoSidedLighting(false);
  renderWindow->AddRenderer(renderer);

  renderer->ClearLights();
  renderer->RemoveAllLights();

  double lightPosition[3] = { 2.0, 2.0, 2.0 };
  double lightFocalPoint[3] = { 0.0, 0.0, 0.0 };
  vtkNew<vtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(lightPosition);
  light->SetPositional(true);
  light->SetConeAngle(60);
  light->SetFocalPoint(lightFocalPoint);
  light->SetIntensity(1.0);
  renderer->AddLight(light);

  double cam_position[3] = { 0.0, 0.0, 3.0 };
  double cam_focal[3] = { 0.5, 0.5, 0.5 };
  double view_up[3] = { 0, 0, 1.0 };
  double cam_parallel_scale = 214;

  vtkCamera* cam = renderer->GetActiveCamera();
  cam->SetPosition(cam_position);
  cam->SetFocalPoint(cam_focal);
  cam->SetViewUp(view_up);
  cam->SetParallelScale(cam_parallel_scale);

  renderer->AddVolume(volume);
  renderWindow->Render();
  iren->Start();

  return 0;
}
