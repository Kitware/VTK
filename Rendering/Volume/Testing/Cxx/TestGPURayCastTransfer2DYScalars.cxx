/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastTransfer2DYScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test 2D transfer function support in GPUVolumeRayCastMapper for multivariate data. The transfer
 * function is manually created and specified over two scalar fields in the input dataset.
 */

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataObject.h"
#include "vtkExodusIIReader.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLImageDataReader.h"

int TestGPURayCastTransfer2DYScalars(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
  vtkNew<vtkExodusIIReader> reader;
  reader->SetFileName(fname);
  reader->SetPointResultArrayStatus("Temp", 1);
  reader->SetPointResultArrayStatus("Pres", 1);

  vtkNew<vtkResampleToImage> resample;
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(200, 200, 200);
  resample->SetInputConnection(reader->GetOutputPort());
  resample->Update();
  delete[] fname;

  // Load the transfer function
  char* tfname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/TestGPURayCastTransfer2DYScalarsTransferFunction.vti");
  vtkNew<vtkXMLImageDataReader> tfReader;
  tfReader->SetFileName(tfname);
  tfReader->Update();
  delete[] tfname;

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetTransferFunction2D(tfReader->GetOutput());
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());
  mapper->SetUseJittering(1);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("Pres");
  mapper->SetTransfer2DYAxisArray("Temp");

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  ren->AddVolume(volume);

  ren->ResetCamera();
  auto cam = ren->GetActiveCamera();
  cam->Azimuth(90);
  cam->Dolly(1.2);

  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren);
}
