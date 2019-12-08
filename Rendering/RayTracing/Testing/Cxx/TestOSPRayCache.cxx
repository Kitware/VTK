/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that caching of time varying data works as expected.
// In particular if the vtkOSPRayCache is working, repeated passes through
// an animation should be much faster than the first because all of the
// ospray data structures are reused.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataGeometryFilter.h"
#include "vtkCutter.h"
#include "vtkInformation.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalDataSetCache.h"
#include "vtkTemporalFractal.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestOSPRayCache(int argc, char* argv[])
{
  auto iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  auto renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(400, 400);

  auto ospray = vtkSmartPointer<vtkOSPRayPass>::New();

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  renderer->SetPass(ospray);

  // a well behaved time varying data source
  auto fractal = vtkSmartPointer<vtkTemporalFractal>::New();
  fractal->SetMaximumLevel(4);
  fractal->DiscreteTimeStepsOn();
  fractal->GenerateRectilinearGridsOff();
  fractal->SetAdaptiveSubdivision(1);
  fractal->TwoDimensionalOff();

  // a slice to test geometry caching
  auto plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0, 0, 0.25);
  plane->SetNormal(0, 0, 1);
  auto cutter = vtkSmartPointer<vtkCutter>::New();
  cutter->SetCutFunction(plane);
  cutter->SetInputConnection(fractal->GetOutputPort());
  auto geom = vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  geom->SetInputConnection(cutter->GetOutputPort());

  // exercise VTK's filter caching too
  auto tcache1 = vtkSmartPointer<vtkTemporalDataSetCache>::New();
  tcache1->SetInputConnection(geom->GetOutputPort());
  tcache1->SetCacheSize(11);

  // draw the slice
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(tcache1->GetOutputPort());
  auto actor = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  // a resample to test volume caching
  auto resample = vtkSmartPointer<vtkResampleToImage>::New();
  resample->SetInputConnection(fractal->GetOutputPort());
  resample->SetSamplingDimensions(50, 50, 50);

  // exercise VTK's filter caching too
  auto tcache2 = vtkSmartPointer<vtkTemporalDataSetCache>::New();
  tcache2->SetInputConnection(resample->GetOutputPort());
  tcache2->SetCacheSize(11);

  // draw the volume
  auto volmap = vtkSmartPointer<vtkSmartVolumeMapper>::New();
  volmap->SetInputConnection(tcache2->GetOutputPort());
  volmap->SetScalarModeToUsePointFieldData();
  volmap->SelectScalarArray("Fractal Volume Fraction");
  auto volprop = vtkSmartPointer<vtkVolumeProperty>::New();
  auto compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  compositeOpacity->AddPoint(0.0, 0.0);
  compositeOpacity->AddPoint(3.0, 1.0);
  volprop->SetScalarOpacity(compositeOpacity);
  auto color = vtkSmartPointer<vtkColorTransferFunction>::New();
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(6.0, 1.0, 1.0, 1.0);
  volprop->SetColor(color);
  auto vol = vtkSmartPointer<vtkVolume>::New();
  vol->SetMapper(volmap);
  vol->SetProperty(volprop);
  renderer->AddViewProp(vol);

  // make the camera sensible
  auto cam = renderer->GetActiveCamera();
  cam->SetPosition(-.37, 0, 8);
  cam->SetFocalPoint(-.37, 0, 0);
  cam->SetViewUp(1, 0, 0);
  cam->Azimuth(-35);

  // now set up the animation over time
  auto info1 = tcache1->GetOutputInformation(0);
  tcache1->UpdateInformation();
  double* tsteps = info1->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int ntsteps = info1->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  auto info2 = tcache2->GetOutputInformation(0);
  tcache2->UpdateInformation();

  // the thing we are trying to test, ospray interface's caching
  vtkOSPRayRendererNode::SetTimeCacheSize(11, renderer);

  // first pass, expected to be comparatively slow
  auto timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  for (int i = 0; i < ntsteps; i += 5)
  {
    double updateTime = tsteps[i];
    cout << "t=" << updateTime << endl;

    info1->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    info2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
    vtkOSPRayRendererNode::SetViewTime(updateTime, renderer);
    renWin->Render();
  }
  timer->StopTimer();
  double etime0 = timer->GetElapsedTime();
  cout << "Elapsed time first renders " << etime0 << endl;

  // subsequent passes, expected to be faster
  timer->StartTimer();
  for (int j = 0; j < 5; ++j)
  {
    for (int i = 0; i < ntsteps; i += 5)
    {
      double updateTime = tsteps[i];
      cout << "t=" << updateTime << endl;

      info1->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
      info2->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), updateTime);
      vtkOSPRayRendererNode::SetViewTime(updateTime, renderer);
      renWin->Render();
    }
  }
  timer->StopTimer();
  double etime1 = timer->GetElapsedTime();
  cout << "Elapsed time for 5 cached render loops " << etime1 << endl;

  if (etime1 > etime0 * 3)
  {
    cerr << "Test failed, 5 rerenders are expected to be faster." << endl;
    cerr << "first render " << etime0 << " vs " << etime1 << " for 5x rerender" << endl;
    return 1;
  }
  iren->Start();

  return 0;
}
