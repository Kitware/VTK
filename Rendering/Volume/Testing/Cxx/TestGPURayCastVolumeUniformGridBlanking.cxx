// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This is a regression test for blanking support of uniform grids with the volume mapper

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkImageDataToUniformGrid.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------------------
int TestGPURayCastVolumeUniformGridBlanking(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkImageDataToUniformGrid> im2ug;
  im2ug->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "RTData");
  im2ug->SetInputConnection(wavelet->GetOutputPort());
  im2ug->Update();

  vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(im2ug->GetOutput());
  int dims[3];
  ug->GetDimensions(dims);
  for (int k = 0; k < (dims[2] - 1) / 4; ++k)
  {
    for (int j = (dims[1] - 1) / 4; j < 3 * (dims[1] - 1) / 4; ++j)
    {
      for (int i = (dims[0] - 1) / 3; i < 2 * (dims[0] - 1) / 3; ++i)
      {
        ug->BlankCell((k * (dims[0] - 1) + j) * (dims[0] - 1) + i);
      }
      for (int i = 3 * dims[0] / 4; i < dims[0]; ++i)
      {
        ug->BlankPoint((k * dims[0] + j) * dims[1] + i);
      }
    }
  }

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(ug);

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 0.05);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pwf);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->Render(); // make sure we have an OpenGL context.

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);
  auto camera = renderer->GetActiveCamera();
  camera->SetPosition(0, 0, 0);
  camera->SetFocalPoint(0, 0.3, 1);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
