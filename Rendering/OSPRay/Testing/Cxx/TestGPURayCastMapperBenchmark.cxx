/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test is intended to benchmark render times for the volumemappers

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkOSPRayPass.h"

// TODO , tweak the sampling rate and number of samples to test if the
// baseline image would be matched (only visible differences on the edges)
//----------------------------------------------------------------------------
int TestGPURayCastMapperBenchmark(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  bool useOSP = true;
  bool useFP = false;
  int EXT=128;
  int RES=900;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useOSP = false;
    }
    if (!strcmp(argv[i], "-FP"))
    {
      useFP = true;
    }
    if (!strcmp(argv[i], "-EXT"))
    {
      EXT = atoi(argv[i+1]);
    }
    if (!strcmp(argv[i], "-RES"))
    {
      RES = atoi(argv[i+1]);
    }
  }

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-(EXT-1), EXT,
                          -(EXT-1), EXT,
                          -(EXT-1), EXT);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  vtkNew<vtkTimerLog> timer;
  cerr << "Make data" << endl;
  timer->StartTimer();
  wavelet->Update();
  timer->StopTimer();
  double makeDataTime = timer->GetElapsedTime();
  cerr << "Make data time: " << makeDataTime << endl;

  vtkNew<vtkGPUVolumeRayCastMapper> gpu_volumeMapper;
  vtkNew<vtkFixedPointVolumeRayCastMapper> cpu_volumeMapper;
  vtkVolumeMapper *volumeMapper = gpu_volumeMapper.GetPointer();
  if (useFP)
  {
    cerr << "USE FP" << endl;
    volumeMapper = cpu_volumeMapper.GetPointer();
  }

  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.3531, 0.2, 0.29, 1);
  ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  ctf->AddRGBPoint(276.829, 0.7, 0.015, 0.15);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(37.3531, 0.0);
  pwf->AddPoint(276.829, 1.0);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(RES, RES);
  renderWindow->Render(); // make sure we have an OpenGL context.

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume.GetPointer());
  renderer->ResetCamera();
  renderWindow->AddRenderer(renderer.GetPointer());

// Attach OSPRay render pass
  vtkNew<vtkOSPRayPass> osprayPass;
  if (useOSP && !useFP)
  {
    renderer->SetPass(osprayPass.GetPointer());
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow.GetPointer());

  int valid = true;
  if (!useFP)
  {
    valid = gpu_volumeMapper->IsRenderSupported(renderWindow.GetPointer(),
                                                volumeProperty.GetPointer());
  }
  int retVal;
  if (valid)
  {
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

    renderer->GetActiveCamera()->SetPosition(0,0,1);
    renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
    renderer->GetActiveCamera()->SetViewUp(0,1,0);
    renderer->ResetCamera();

    renderWindow->SetSize(300, 300);
    renderWindow->Render();

    iren->Initialize();

    retVal = vtkRegressionTestImage( renderWindow.GetPointer() );
    if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
  }
  else
  {
    retVal = vtkTesting::PASSED;
    cout << "Required extensions not supported." << endl;
  }

  return !((retVal == vtkTesting::PASSED) ||
           (retVal == vtkTesting::DO_INTERACTOR));
}
