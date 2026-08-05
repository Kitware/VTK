// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredPointsReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * Description
 * This is a test for clipping of volume using the camera near plane when the
 * camera is inside the volume. The test renders the ironProt dataset and
 * dollies the camera inside the dataset.
 */
int TestAnariCameraInside(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);
  mapper->SetSampleDistance(1.0);

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

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300);
  renWin->SetMultiSamples(0);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariCameraInside");

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> ren;
  ren->AddVolume(volume);
  renWin->AddRenderer(ren);

  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(32.569345, 24.294325, 46.161609);
  ren->GetActiveCamera()->SetFocalPoint(33.943576, 32.758459, 31.814997);
  ren->GetActiveCamera()->SetViewUp(-0.066044, 0.996865, -0.0435661);
  renWin->Render();

  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renWin);
  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
    return !retVal;
  }

  vtkLogF(WARNING, "Required feature KHR_SPATIAL_FIELD_STRUCTURED_REGULAR not supported.");
  return VTK_SKIP_RETURN_CODE;
}
