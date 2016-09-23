/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastGradientOpacityLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests gradient opacity TF support when combined with vtkLightKit.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkLightKit.h"


int TestGPURayCastGradientOpacityLight(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  ren->SetBackground(0.1, 0.4, 0.2);

  // Setup lights
  vtkNew<vtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
   "Data/HeadMRVolume.mhd");

  vtkNew<vtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // Prepare TFs
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

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pwf.GetPointer());
  volumeProperty->SetGradientOpacity(gf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->ShadeOn();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  ren->AddVolume(volume.GetPointer());
  volume->RotateX(-30);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin.GetPointer(), 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) ||
           (retVal == vtkTesting::DO_INTERACTOR));
}
