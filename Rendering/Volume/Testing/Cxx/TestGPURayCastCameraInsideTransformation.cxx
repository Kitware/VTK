/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastCameraInsideRotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/// Description:
/// Tests clipping of a rotated volume (vtkProp3D::GetMatrix) using the camera's
/// near plane while the camera is inside the volume.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
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


static const char* TestGPURayCastCameraInsideTransformationLog =
"# StreamVersion 1\n"
"EnterEvent 298 27 0 0 0 0 0\n"
"MouseWheelForwardEvent 200 142 0 0 0 0 0\n"
"LeaveEvent 311 71 0 0 0 0 0\n";

int TestGPURayCastCameraInsideTransformation(int argc, char* argv[])
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
  pf->AddPoint(500,  0.02);
  pf->AddPoint(1000, 0.02);
  pf->AddPoint(1150, 0.85);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0,   0.0);
  gf->AddPoint(90,  0.5);
  gf->AddPoint(100, 0.7);

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

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());
  ren->AddVolume(volume.GetPointer());

  // Set a vtkProp3D transformation
  volume->RotateX(180);
  volume->RotateY(85);
  volume->RotateZ(55);
  volume->SetOrigin(300, 20, 30);

  // Prepare the camera to be inside the volume
  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(308.423, 120.943, -142.038);

  // Initialize interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  renWin->Render();
  iren->Initialize();

  int rv = vtkTesting::InteractorEventLoop(argc, argv, iren.GetPointer(),
    TestGPURayCastCameraInsideTransformationLog);
  return rv;
}
