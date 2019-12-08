/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test is intended to benchmark render times for the vtkGPURayCastMapper

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
int TestGPURayCastMapperBenchmark(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-127, 128, -127, 128, -127, 128);
  wavelet->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

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

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(900, 900);
  renderWindow->Render(); // make sure we have an OpenGL context.

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  int valid = volumeMapper->IsRenderSupported(renderWindow, volumeProperty);
  int retVal;
  if (valid)
  {

    vtkNew<vtkTimerLog> timer;
    timer->StartTimer();
    renderWindow->Render();
    timer->StopTimer();
    double firstRender = timer->GetElapsedTime();
    cerr << "First Render Time: " << firstRender << endl;

    int numRenders = 20;
    for (int i = 0; i < numRenders; ++i)
    {
      renderer->GetActiveCamera()->Azimuth(1);
      renderer->GetActiveCamera()->Elevation(1);
      renderWindow->Render();
    }

    timer->StartTimer();
    numRenders = 100;
    for (int i = 0; i < numRenders; ++i)
    {
      renderer->GetActiveCamera()->Azimuth(1);
      renderer->GetActiveCamera()->Elevation(1);
      renderer->GetActiveCamera()->OrthogonalizeViewUp();
      renderWindow->Render();
    }
    timer->StopTimer();
    double elapsed = timer->GetElapsedTime();
    cerr << "Interactive Render Time: " << elapsed / numRenders << endl;

    renderer->GetActiveCamera()->SetPosition(0, 0, 1);
    renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    renderer->ResetCamera();

    renderWindow->SetSize(300, 300);
    renderWindow->Render();

    iren->Initialize();

    retVal = vtkRegressionTestImage(renderWindow);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
