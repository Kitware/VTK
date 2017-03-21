/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastImageSampleXY.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * \brief Tests image sample distance (XY resolution) of a volume (ray-cast)
 * rendering.
 *
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkConeSource.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageResize.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume16Reader.h"


int TestGPURayCastImageSampleXY(int argc, char* argv[])
{
  //cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  vtkNew<vtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(3.2, 3.2, 1.5);

  // Upsample data
  vtkNew<vtkImageResize> resample;
  resample->SetInputConnection(reader->GetOutputPort());
  resample->SetResizeMethodToOutputDimensions();
  resample->SetOutputDimensions(128, 128, 128);
  resample->Update();

  // Setup mappers, properties and actors
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0,    0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500,  0.1, 1.0, 0.3);
  ctf->AddRGBPoint(1000, 0.1, 1.0, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0,    0.00);
  pf->AddPoint(500,  0.15);
  pf->AddPoint(1000, 0.15);
  pf->AddPoint(1150, 0.85);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0,   0.0);
  gf->AddPoint(90,  0.5);
  gf->AddPoint(100, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pf.GetPointer());
  volumeProperty->SetGradientOpacity(gf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  // Downsample volume-rendered image (cast 1 ray for a 4x4 pixel kernel)
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());
  mapper->SetUseJittering(0);
  mapper->SetImageSampleDistance(8.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Without down-sampling
  vtkNew<vtkGPUVolumeRayCastMapper> mapper2;
  mapper2->SetInputConnection(resample->GetOutputPort());
  mapper2->SetUseJittering(0);
  mapper2->SetImageSampleDistance(1.0);

  vtkNew<vtkVolume> volume2;
  volume2->SetMapper(mapper2.GetPointer());
  volume2->SetProperty(volumeProperty.GetPointer());

  vtkNew<vtkConeSource> coneSource;
  coneSource->SetResolution(20);
  coneSource->SetHeight(280);
  coneSource->SetRadius(40.0);
  coneSource->SetCenter(110.0, 70.0, 30.0);
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper.GetPointer());

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.3, 0.3, 0.5);
  ren->SetViewport(0.0, 0.0, 0.5, 0.5);
  ren->AddVolume(volume.GetPointer());
  ren->AddActor(coneActor.GetPointer());
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderer> ren2;
  ren2->SetBackground(0., 0., 0.);
  ren2->SetViewport(0.0, 0.5, 0.5, 1.0);
  ren2->SetActiveCamera(ren->GetActiveCamera());
  ren2->AddVolume(volume.GetPointer());
  ren2->AddActor(coneActor.GetPointer());
  renWin->AddRenderer(ren2.GetPointer());

  vtkNew<vtkRenderer> ren3;
  ren3->SetBackground(0., 0., 0.);
  ren3->SetViewport(0.5, 0.0, 1.0, 0.5);
  ren3->SetActiveCamera(ren->GetActiveCamera());
  ren3->AddVolume(volume.GetPointer());
  ren3->AddActor(coneActor.GetPointer());
  renWin->AddRenderer(ren3.GetPointer());

  vtkNew<vtkRenderer> ren4;
  ren4->SetBackground(0.3, 0.3, 0.5);
  ren4->SetViewport(0.5, 0.5, 1.0, 1.0);
  ren4->SetActiveCamera(ren->GetActiveCamera());
  ren4->AddVolume(volume2.GetPointer());
  ren4->AddActor(coneActor.GetPointer());
  renWin->AddRenderer(ren4.GetPointer());

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(-10);
  ren->GetActiveCamera()->Elevation(130);
  ren->GetActiveCamera()->Zoom(1.6);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin.GetPointer(), 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !((retVal == vtkTesting::PASSED) ||
           (retVal == vtkTesting::DO_INTERACTOR));
}
