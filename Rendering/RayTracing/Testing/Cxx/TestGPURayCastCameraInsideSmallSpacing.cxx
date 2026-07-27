// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a test for clipping of volume using the camera near plane when the
// camera is inside the volume. The test renders the ironProt dataset after
// changing it to have a very small spacing and dollies the camera inside the
// volume geometry.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageChangeInformation.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastCameraInsideSmallSpacing(int argc, char* argv[])
{
  vtkLogF(INFO, "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)");

  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      vtkLogF(ERROR, "GL");
      useOSP = false;
    }
  }

  char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  int dims[3];
  reader->Update();
  reader->GetOutput()->GetDimensions(dims);

  double desiredBounds = 0.0005;
  double desiredSpacing[3];
  for (int i = 0; i < 3; ++i)
  {
    desiredSpacing[i] = desiredBounds / static_cast<double>(dims[i]);
  }

  vtkNew<vtkImageChangeInformation> imageChangeInfo;
  imageChangeInfo->SetInputConnection(reader->GetOutputPort());
  imageChangeInfo->SetOutputSpacing(desiredSpacing);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(imageChangeInfo->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);
  mapper->SetSampleDistance(7e-6);

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  color->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(255.0, 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetColor(color);
  property->SetScalarOpacity(opacity);
  property->SetInterpolationTypeToLinear();
  property->ShadeOff();
  property->SetScalarOpacityUnitDistance(7e-6);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  // Attach OSPRay render pass
  vtkSmartPointer<vtkOSPRayPass> osprayPass = vtkSmartPointer<vtkOSPRayPass>::New();
  if (useOSP)
  {
    ren->SetPass(osprayPass);
  }

  ren->AddVolume(volume);
  ren->GetActiveCamera()->SetPosition(0.000356, 0.000303, 0.000389);
  ren->GetActiveCamera()->SetFocalPoint(0.000262, 0.000190, 0.000250);
  ren->GetActiveCamera()->SetViewUp(-0.217149, 0.823645, -0.523885);
  ren->ResetCameraClippingRange();

  return vtkRegressionTestImage(renWin) == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
