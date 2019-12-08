/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastTransfer2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test 2D transfer function support in GPUVolumeRayCastMapper.  The transfer
 * function is created manually using known value/gradient histogram information
 * of the test data (tooth.hdr). A filter to create these histograms will be
 * added in the future.
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkNrrdReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

typedef vtkSmartPointer<vtkImageData> Transfer2DPtr;
Transfer2DPtr Create2DTransfer()
{
  int bins[2] = { 256, 256 };
  Transfer2DPtr image = Transfer2DPtr::New();
  image->SetDimensions(bins[0], bins[1], 1);
  image->AllocateScalars(VTK_FLOAT, 4);
  vtkFloatArray* arr = vtkFloatArray::SafeDownCast(image->GetPointData()->GetScalars());

  // Initialize to zero
  void* dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, bins[0] * bins[1] * 4 * sizeof(float));

  // Setting RGBA [1.0, 0,0, 0.05] for a square in the histogram (known)
  // containing some of the interesting edges (e.g. tooth root).
  for (int j = 0; j < bins[1]; j++)
    for (int i = 0; i < bins[0]; i++)
    {
      if (i > 130 && i < 190 && j < 50)
      {
        double const jFactor = 256.0 / 50;

        vtkIdType const index = bins[0] * j + i;
        double const red = static_cast<double>(i) / bins[0];
        double const green = jFactor * static_cast<double>(j) / bins[1];
        double const blue = jFactor * static_cast<double>(j) / bins[1];
        double const alpha = 0.25 * jFactor * static_cast<double>(j) / bins[0];

        double color[4] = { red, green, blue, alpha };
        arr->SetTuple(index, color);
      }
    }

  return image;
}

////////////////////////////////////////////////////////////////////////////////
int TestGPURayCastTransfer2D(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  vtkNew<vtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(510, 0.4, 0.4, 1.0);
  ctf->AddRGBPoint(640, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 0.9, 0.1, 0.1);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.4);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(range[1] / 4.0, 1.0);

  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);

  // Prepare 2D Transfer Functions
  Transfer2DPtr tf2d = Create2DTransfer();

  volumeProperty->SetTransferFunction2D(tf2d);

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Elevation(-90.0);
  ren->GetActiveCamera()->Zoom(1.4);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render();

  // Simulate modification of 2D transfer function to test for shader issues
  tf2d->Modified();
  renWin->Render();

  int retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
