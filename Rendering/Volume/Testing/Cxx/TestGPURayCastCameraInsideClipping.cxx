/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Tests that VolumeRayCastMapper::IsCameraInside correctly detects if the
 * camera is clipping part of the proxy geometry (either by being inside the
 * bbox or by being close enough). This test positions the camera exactly at
 * a point where a corner of the proxy geometry falls behind the near plane
 * thus clipping those fragments and the volume image chunk sampled by those
 * rays.
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"


int TestGPURayCastCameraInsideClipping(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren1;
  renWin->AddRenderer(ren1.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                     "Data/HeadMRVolume.mhd");
  vtkNew<vtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  // Volume
  vtkNew<vtkGPUVolumeRayCastMapper> mapper1;
  mapper1->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(1.0, 0.095, 0.33, 0.82);
  ctf->AddHSVPoint(53.3, 0.04, 0.7, 0.63);
  ctf->AddHSVPoint(256, 0.095, 0.33, 0.82);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(4.48, 0.0);
  pwf->AddPoint(43.116, 1.0);
  pwf->AddPoint(641.0, 1.0);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(10, 0.0);
  gf->AddPoint(70, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty1;
  volumeProperty1->SetScalarOpacity(pwf.GetPointer());
  volumeProperty1->SetColor(ctf.GetPointer());
  volumeProperty1->SetDisableGradientOpacity(1);
  volumeProperty1->SetInterpolationTypeToLinear();
  volumeProperty1->ShadeOn();

  vtkNew<vtkVolume> volume1;
  volume1->SetMapper(mapper1.GetPointer());
  volume1->SetProperty(volumeProperty1.GetPointer());

  // Sphere
  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(20.0);
  sphere->SetThetaResolution(20.0);
  sphere->SetCenter(90, 60, 100);
  sphere->SetRadius(40.0);
  sphere->Update();
  vtkNew<vtkPolyDataMapper> pMapper;
  pMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereAct;
  sphereAct->SetMapper(pMapper.GetPointer());

  // Outline
  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(reader->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper.GetPointer());

  ren1->AddVolume(volume1.GetPointer());
  ren1->AddActor(sphereAct.GetPointer());
  ren1->AddActor(outlineActor.GetPointer());

  ren1->GetActiveCamera()->SetFocalPoint(94,142,35);
  ren1->GetActiveCamera()->SetPosition(94,142,200);
  ren1->GetActiveCamera()->SetViewAngle(110);
  ren1->ResetCameraClippingRange();
  renWin->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renWin->GetInteractor()->SetInteractorStyle(style.GetPointer());

  ren1->GetActiveCamera()->Elevation(-45);
  ren1->GetActiveCamera()->OrthogonalizeViewUp();

  ren1->GetActiveCamera()->Azimuth(34.9);
  ren1->GetActiveCamera()->OrthogonalizeViewUp();
  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin.GetPointer(), 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) ||
           (retVal == vtkTesting::DO_INTERACTOR));
}
