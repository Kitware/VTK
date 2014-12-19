/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartVolumeMapperGradientOpacity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This code volume renders the torso dataset and tests the gradient opacity
// function support for volume mappers

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

int TestSmartVolumeMapperGradientOpacity(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0.0, 0.0, 0.5, 1.0);
  renWin->AddRenderer(ren1.GetPointer());
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(ren2.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                     "Data/HeadMRVolume.mhd");

  vtkNew<vtkMetaImageReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete [] fname;

  vtkNew<vtkSmartVolumeMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddHSVPoint(1.0, 0.095, 0.33, 0.82);
  ctf->AddHSVPoint(53.3, 0, 1, 0.36);
  ctf->AddHSVPoint(256, 0.095, 0.33, 0.82);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(4.48, 0.0);
  pwf->AddPoint(43.116, 1.0);
  pwf->AddPoint(641.0, 1.0);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(5, 0.0);
  gf->AddPoint(70, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty1;
  volumeProperty1->SetScalarOpacity(pwf.GetPointer());
  volumeProperty1->SetColor(ctf.GetPointer());
  volumeProperty1->ShadeOn();

  vtkNew<vtkVolume> volume1;
  volume1->SetMapper(mapper.GetPointer());
  volume1->SetProperty(volumeProperty1.GetPointer());
  ren1->AddVolume(volume1.GetPointer());

  vtkNew<vtkVolumeProperty> volumeProperty2;
  volumeProperty2->SetScalarOpacity(pwf.GetPointer());
  volumeProperty2->SetColor(ctf.GetPointer());
  volumeProperty2->SetGradientOpacity(gf.GetPointer());
  volumeProperty2->ShadeOn();

  vtkNew<vtkVolume> volume2;
  volume2->SetMapper(mapper.GetPointer());
  volume2->SetProperty(volumeProperty2.GetPointer());
  ren2->AddVolume(volume2.GetPointer());

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin.GetPointer(), 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
