/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastTextureStreaming.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description:
// Tests infrastructure to volume render a 3D texture bigger than the available
// graphics memory by splitting it and individually streaming each block (bricking)
// into the GPU.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkVolume16Reader.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkImageResize.h"
#include "vtkImageResample.h"

#include "vtkMatrix4x4.h"
#include "vtkImageData.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkAbstractMapper.h"


int TestGPURayCastTextureStreaming(int argc, char* argv[])
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
  resample->SetOutputDimensions(512, 512, 512);
  resample->Update();

  // Prepare TFs
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0,    0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500,  1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
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

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  ren->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(resample->GetOutputPort());
  mapper->SetUseJittering(0);

  // Force a number of partition blocks
  vtkOpenGLGPUVolumeRayCastMapper* mappergl =
    vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper.GetPointer());
  mappergl->SetPartitions(2, 1, 2);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  ren->AddVolume(volume.GetPointer());

  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);

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
