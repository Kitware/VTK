/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartVolumeMapperWindowLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the smart volume mapper and composite method with a
// customed window/level values (brightness/contrast).
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMetaImageReader.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkTestUtilities.h"
#include "vtkVolumeProperty.h"

int TestSmartVolumeMapperWindowLevel(int argc,
                                     char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 401);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

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
  ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(98.70, 169/255.0, 176/255.0, 107/255.0);
  ctf->AddRGBPoint(269.04, 100/255.0, 116/255.0, 191/255.0);
  ctf->AddRGBPoint(449.88, 0.0, 128/255.0, 174/255.0);
  ctf->AddRGBPoint(641.00, 1.0, 1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0.0, 0.0);
  pwf->AddPoint(36.05, 0.0);
  pwf->AddPoint(218.30, 0.17);
  pwf->AddPoint(412.41, 1.0);
  pwf->AddPoint(641.0, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pwf.GetPointer());
  volumeProperty->SetColor(ctf.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  ren->AddVolume(volume.GetPointer());

  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin.GetPointer(), 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
