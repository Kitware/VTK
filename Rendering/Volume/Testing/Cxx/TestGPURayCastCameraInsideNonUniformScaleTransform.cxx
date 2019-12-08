/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideNonUniformScaleTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/// Description:
/// Test for the case when the camera is inside the bounding box of the volume
/// with a uneven scale transformation (diagonal values not same) on the prop.
/// To accentuate the issue, a large view angle is applied.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastCameraInsideNonUniformScaleTransform(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  vtkNew<vtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(1, 1, 1);

  double elements[16] = { 3.2, 0, 0, 200, 0, 3.2, 0, 100, 0, 0, 1.5, 40, 0, 0, 0, 1 };

  vtkNew<vtkMatrix4x4> matrix;
  matrix->DeepCopy(elements);

  // Prepare TFs
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.02);
  pf->AddPoint(1000, 0.02);
  pf->AddPoint(1150, 0.85);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.5);
  gf->AddPoint(100, 0.7);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);
  volumeProperty->ShadeOn();

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  volume->PokeMatrix(matrix);
  ren->AddVolume(volume);

  // Prepare the camera to be inside the volume
  ren->ResetCamera();
  vtkCamera* cam = ren->GetActiveCamera();
  cam->SetViewAngle(170);
  cam->SetPosition(256.846, 168.853, 38.7375);
  cam->SetFocalPoint(178.423, 110.943, 142.038);
  cam->SetViewUp(-0.105083, 0.899357, 0.424399);
  ren->ResetCameraClippingRange();

  // Initialize interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
