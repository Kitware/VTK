// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Designed to test paraview/paraview#19012: when the array to volume render
 * with is changed, the volume mapper must update correctly.
 */

#include <vtkArrayCalculator.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkTestingObjectFactory.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

int TestMultiVariateToRGBA(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> controller;
#else
  vtkNew<vtkDummyController> controller;
#endif

  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName((dataRoot + "/Data/Ce_Co_Fe_Gd_small.vti").c_str());

  vtkNew<vtkMultiVariateToRGBA> colorFilter;
  colorFilter->SetInputConnection(reader->GetOutputPort());
  colorFilter->Update();

  // find opacity range
  vtkImageData* data = vtkImageData::SafeDownCast(colorFilter->GetOutputDataObject(0));
  vtkDataArray* colors = data->GetPointData()->GetArray("rgba");
  double* opacityRange = colors->GetRange(0);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(colorFilter->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("rgba");

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(opacityRange[0], 0.0);
  scalarOpacity->AddPoint(opacityRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetIndependentComponents(false);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  iren->SetInteractorStyle(style);

  renderWindow->Render();
  renderer->ResetCamera();
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Azimuth(180);
  camera->Zoom(1.5);
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  controller->Finalize();
  return EXIT_SUCCESS;
}
